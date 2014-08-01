#include <vector>

#include "../protobuf/connect.pb.h"
#include "../protobuf/disconn.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/setmonster.pb.h"
#include "../protobuf/erasemonster.pb.h"
#include "../protobuf/peacemove.pb.h"

#include "Check_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "msg.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"

#include "Memory_Pool.h"
#include "TimerThread.h"
#include "monster.h"
#include "DMap_monster.h"

using namespace std;

void printLog(const char *msg, ...);
void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents);
void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void make_vector_id_in_room_except_me(Character*, vector<Character *>&, bool);
void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);

void Handler_HELLOWORLD(LPPER_IO_DATA ioInfo, std::string* readContents);
void Handler_PEACEMOVE(LPPER_IO_DATA, std::string*);

bool Boundary_Check(int, const int,const int, int, int);


void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents)
{
	Sock_set *sock_set = Sock_set::getInstance();
	auto FVEC = F_Vector::getInstance();
	auto AMAP = Access_Map::getInstance();
	auto CMap = Check_Map::getInstance();
	F_Vector_Mon *FVEC_M = F_Vector_Mon::getInstance();
	Access_Map_Mon * AMAP_M = Access_Map_Mon::getInstance();

	CONNECT::CONTENTS connect;
	INIT::CONTENTS initContents;
	SET_USER::CONTENTS setuserContents;
	SET_MONSTER::CONTENTS setmonsterContents;

	string bytestring;
	int len;
	vector<Character *> receiver;
	vector<Character *> me;
	vector<Monster *> vec_mon;

	connect.ParseFromString(*readContents);
	if (connect.data() != "HELLO SERVER!")
	{
		//가짜 클라이언트
	}
	int char_id;
	int x, y, lv, maxHp, power, exp;
	static int id = 0;

	// 캐릭터 객체를 생성 후
	// 캐릭터 생성하고 init 하는 것에 대해선 char lock할 필요가 없다.
	char_id = InterlockedIncrement((unsigned *)&id);
	Character* myChar = new Character(char_id);
	myChar->setLv(1, HpPw[0][0], HpPw[0][1]);
	myChar->setExp(0);
	myChar->setSock(handleInfo->hClntSock);
	me.push_back(myChar);

	x = myChar->getX();
	y = myChar->getY();
	lv = myChar->getLv();
	maxHp = myChar->getMaxHp();
	power = myChar->getPower();
	exp = myChar->getExp();

	ioInfo->id = char_id;
	ioInfo->myCharacter = myChar;
	
	E_List* elist = FVEC->get(x, y);

	{
		Scoped_Wlock SW1(&AMAP->slock);
		Scoped_Wlock SW2(&elist->slock);
		AMAP->insert(char_id, myChar);
		CMap->insert(handleInfo->hClntSock, char_id);
		sock_set->erase(handleInfo->hClntSock);
		elist->push_back(myChar);
	}

	// x와 y의 초기값을 가져온다.   
	{
		auto myData = initContents.mutable_data()->Add();
		myData->set_id(char_id);
		myData->set_x(x);
		myData->set_y(y);
		myData->set_lv(lv);
		myData->set_maxhp(maxHp);
		myData->set_power(power);
		myData->set_exp(exp);
	}

	initContents.SerializeToString(&bytestring);
	len = bytestring.length();

	{
		Scoped_Rlock SR(&elist->slock);
		send_message(msg(PINIT, len, bytestring.c_str()), me, true);
	}
	receiver.clear();
	bytestring.clear();
	initContents.clear_data();

	// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.
	{
		auto myData = setuserContents.mutable_data()->Add();
		myData->set_id(char_id);
		myData->set_x(x);
		myData->set_y(y);
		myData->set_lv(lv);
		myData->set_maxhp(maxHp);
		myData->set_power(power);
		myData->set_exp(exp);
	}

	setuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	{
		Scoped_Rlock SR(&elist->slock);
		
		// 내가 있는 방에 있는 친구들에게 내가 등장함을 알린다.
		// 나와 같은방에 있는 친구들은 누구?
		make_vector_id_in_room_except_me(myChar, receiver, false/*autolock*/);

		send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, false);	
		// 이제 생성한 char에 대해서 자료구조에 넣어주었고 내가등장함을 다른 유저에게 알렸다. 이제부턴 char 에대해서 lock을 해줘야 겠다.
		// 근데 해줄 곳이 없네.. 캐릭터를 read write 하는 곳에 해야하는데 그런 곳이 없으니. 내 판단 맞나요?

		setuserContents.clear_data();
		bytestring.clear();

		// PCONNECT로 접속한 유저에게 같은 방에있는 유저들의 정보를 전송한다.
		for (int i = 0; i < receiver.size(); i++) {
			auto tmpChar = receiver[i];

			auto tempData = setuserContents.mutable_data()->Add();
			tempData->set_id(tmpChar->getID());
			tempData->set_x(tmpChar->getX());
			tempData->set_y(tmpChar->getY());
			tempData->set_lv(lv);
			tempData->set_maxhp(maxHp);
			tempData->set_power(power);
			tempData->set_exp(exp);

			if (setuserContents.data_size() == SET_USER_MAXIMUM) // SET_USER_MAXIMUM이 한계치로 접근하려고 할 때
			{
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PSET_USER, len, bytestring.c_str()), me, false);

				setuserContents.clear_data();
				bytestring.clear();
			}
		}
		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), me, true);
		
		receiver.clear();
		setuserContents.clear_data();
		bytestring.clear();
	}

	//같은방에 있는 몬스터의 정보를 전송한다.
	E_List_Mon* elist_m = FVEC_M->get(x, y);
	{
		Scoped_Rlock SR(&elist_m->slock);
		make_monster_vector_in_room(myChar, vec_mon, false);
		for (int i = 0; i < vec_mon.size(); ++i)
		{
			Monster* tmpMon = vec_mon[i];
			auto setmon = setmonsterContents.add_data();
			setmon->set_id(tmpMon->getID());
			setmon->set_x(tmpMon->getX()); 
			setmon->set_y(tmpMon->getY());
			setmon->set_name(tmpMon->getName());
			setmon->set_lv(tmpMon->getLv());
			setmon->set_maxhp(tmpMon->getMaxHp());
			setmon->set_power(tmpMon->getPower());
			setmon->set_exp(tmpMon->getExp());
		}
		setmonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_MON, len, bytestring.c_str()), me, false);

		vec_mon.clear();
		setmonsterContents.clear_data();
		bytestring.clear();
	}
}

void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents)
{
	MOVE_USER::CONTENTS moveuserContents;
	ERASE_USER::CONTENTS eraseuserContents;
	SET_USER::CONTENTS setuserContents;
	ERASE_MONSTER::CONTENTS erasemonsterContents;
	SET_MONSTER::CONTENTS setmonsterContents;
	std::string bytestring;

	auto Amap = Access_Map::getInstance();
	auto FVEC = F_Vector::getInstance();
	auto FVEC_M = F_Vector_Mon::getInstance();

	int x = pCharacter->getX(), y = pCharacter->getY();
	E_List* elist = FVEC->get(x, y);

	vector<Character*> charId_in_room_except_me;
	vector<Character *> me;
	me.push_back(pCharacter);
	vector<Monster *> vec_mon;
	
	moveuserContents.ParseFromString(*readContents);

	int cur_id, x_off, y_off;
	int len;

	auto user = moveuserContents.data(0);
	cur_id = user.id();
	x_off = user.xoff();
	y_off = user.yoff();
	moveuserContents.clear_data();

	/* 경계값 체크 로직 */
	if (Boundary_Check(cur_id, x, y, x_off, y_off) == false) {
		return;
	}

	// 지금 내가 있는 방에 몬스터가 있는지 확인을 하도록 하자. 몬스터와 함께있으면 못움직이게 할 것이기 때문.
	{
		E_List_Mon* elist_m = FVEC_M->get(x, y);
		Scoped_Rlock SR(&elist_m->slock);
		if (!elist_m->empty())
		{
			auto moveuser = moveuserContents.add_data();
			moveuser->set_id(cur_id);
			moveuser->set_xoff(0);
			moveuser->set_yoff(0);
			
			moveuserContents.SerializeToString(&bytestring);
			len = bytestring.size();

			send_message(msg(PMOVE_USER, len, bytestring.c_str()), me, true);

			eraseuserContents.clear_data();
			bytestring.clear();
			return;
		}
	}

	// 기존의 방의 유저들의 정보를 삭제함

	// 나와 같은방에 있는 친구들은 누구?
	{
		Scoped_Rlock SR(&elist->slock);
		Scoped_Rlock SRU(pCharacter->getLock());
		make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

		for (int i = 0; i < charId_in_room_except_me.size(); ++i)
		{
			auto eraseuser = eraseuserContents.add_data();
			eraseuser->set_id(charId_in_room_except_me[i]->getID());

			if (eraseuserContents.data_size() == ERASE_USER_MAXIMUM) // ERASE_USER_MAXIMUM이 한계치로 접근하려고 할 때
			{
				eraseuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PERASE_USER, len, bytestring.c_str()), me, true);

				eraseuserContents.clear_data();
				bytestring.clear();
			}
		}

		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		// 기존 방의 유저들의 정보를 지운다.
		send_message(msg(PERASE_USER, len, bytestring.c_str()), me, true);

		bytestring.clear();
		eraseuserContents.clear_data();

		// 기존 방의 유저들에게 내가 사라짐을 알림
		auto eraseuser = eraseuserContents.add_data();
		eraseuser->set_id(cur_id);
		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_USER, len, bytestring.c_str()), charId_in_room_except_me, true);

		// 기존 방의 몬스터들의 정보를 지운다.
		E_List_Mon* elist_m = FVEC_M->get(x, y);

		Scoped_Rlock SRM(&elist_m->slock);

		make_monster_vector_in_room(pCharacter, vec_mon, false);
		
		for (int i = 0; i < vec_mon.size(); ++i)
		{
			auto erasemon = erasemonsterContents.add_data();
			erasemon->set_id(vec_mon[i]->getID());
		}
		erasemonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_MON, len, bytestring.c_str()), me, true);

		bytestring.clear();
		erasemonsterContents.clear_data();
	}
	
	charId_in_room_except_me.clear();
	vec_mon.clear();

	// 캐릭터를 해당 좌표만큼 이동시킴
	{
		Scoped_Wlock SW(&elist->slock);
		elist->erase(cur_id);
	}
	int newX = x + x_off, newY = y + y_off;
	{
		Scoped_Wlock SWU(pCharacter->getLock());
		pCharacter->setX(newX);
		pCharacter->setY(newY);
	}
	elist = FVEC->get(newX, newY);
	{
		Scoped_Wlock SW(&elist->slock);
		elist->push_back(pCharacter);
	}

	//실제로 나를 움직임을 보냄.
	{
		auto moveuser = moveuserContents.add_data();
		moveuser->set_id(cur_id);
		moveuser->set_xoff(x_off);
		moveuser->set_yoff(y_off);
	}
	moveuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	send_message(msg(PMOVE_USER, len, bytestring.c_str()), me, false);

	bytestring.clear();
	moveuserContents.clear_data();

	// 나와 같은방에 있는 친구들은 누구?
	{
		Scoped_Rlock SR(&elist->slock);
		Scoped_Rlock SRU(pCharacter->getLock());
		make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

		// 새로운 방의 유저들에게 내가 등장함을 알림
		x = pCharacter->getX(), y = pCharacter->getY();
		int lv = pCharacter->getLv(), maxHp = pCharacter->getMaxHp(), power = pCharacter->getPower(), exp = pCharacter->getExp();
		auto setuser = setuserContents.add_data();
		setuser->set_id(cur_id);
		setuser->set_x(x);
		setuser->set_y(y);
		setuser->set_lv(lv);
		setuser->set_maxhp(maxHp);
		setuser->set_power(power);
		setuser->set_exp(exp);

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), charId_in_room_except_me, false);

		bytestring.clear();
		setuserContents.clear_data();

		for (int i = 0; i < charId_in_room_except_me.size(); ++i)
		{
			auto setuser = setuserContents.add_data();
			auto tmpChar = charId_in_room_except_me[i];
			setuser->set_id(tmpChar->getID());
			setuser->set_x(tmpChar->getX());
			setuser->set_y(tmpChar->getY());
			setuser->set_lv(tmpChar->getLv());
			setuser->set_maxhp(tmpChar->getMaxHp());
			setuser->set_power(tmpChar->getPower());
			setuser->set_exp(tmpChar->getExp());

			if (setuserContents.data_size() == SET_USER_MAXIMUM) {
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PSET_USER, len, bytestring.c_str()), me, true);

				setuserContents.clear_data();
				bytestring.clear();
			}
		}

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), me, true);

		bytestring.clear();
		setuserContents.clear_data();
		
		// 새로운 방의 몬스터들의 정보를 가져온다.
		E_List_Mon* elist_m = FVEC_M->get(x, y);
		Scoped_Rlock SR_M(&elist_m->slock);
		make_monster_vector_in_room(pCharacter, vec_mon, false);

		for (int i = 0; i < vec_mon.size(); ++i)
		{
			Monster* tmpMon = vec_mon[i];
			//tmpMon->setState(BATTLE);

			auto setmon = setmonsterContents.add_data();
			setmon->set_id(tmpMon->getID());
			setmon->set_x(tmpMon->getX());
			setmon->set_y(tmpMon->getY());
			setmon->set_name(tmpMon->getName());
			setmon->set_lv(tmpMon->getLv());
			setmon->set_maxhp(tmpMon->getMaxHp());
			setmon->set_power(tmpMon->getPower());
			setmon->set_exp(tmpMon->getExp());

		}
		setmonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_MON, len, bytestring.c_str()), me, true);

		bytestring.clear();
		setmonsterContents.clear_data();
	}


	charId_in_room_except_me.clear();
	vec_mon.clear();

	printLog("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);

}

void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents)
{
	DISCONN::CONTENTS disconn;

	disconn.ParseFromString(*readContents);
	if (disconn.data() != "BYE SERVER!")
	{
		//가짜 클라이언트
	}

	printLog("Nomal turn off\n");
	remove_valid_client(handleInfo, ioInfo);
}

void Handler_HELLOWORLD(LPPER_IO_DATA ioInfo, std::string* readContents) {
	auto MemoryPool = Memory_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	if (ioInfo->block != nullptr) {
		MemoryPool->pushBlock(ioInfo->block);
	}
	ioInfoPool->pushBlock(ioInfo);
	printLog("Hello\n");
	
	auto timer = Timer::getInstance();

	char *str = "Hello World!";
	int type = PHELLOWORLD, len = strlen(str);

	char arr[25];

	memcpy(arr, &type, sizeof(int));
	memcpy(arr + sizeof(int), &len, sizeof(int));
	memcpy(arr + 2 * sizeof(int), str, len);
	timer->addSchedule(1000, string(arr,len+2*sizeof(int)));
}

void Handler_PEACEMOVE(LPPER_IO_DATA ioInfo, std::string* readContents) {
	PEACEMOVE::CONTENTS peacemove;

	peacemove.ParseFromString(*readContents);
	int ID = peacemove.id();

	auto AMAP_MON = Access_Map_Mon::getInstance();
	Monster* monster;
	{
		Scoped_Rlock SR(&AMAP_MON->slock);
		monster = AMAP_MON->find(ID);
	}
	
	{
		Scoped_Wlock SW(monster->getLock());
		int bef_x_off, bef_y_off;
		
		if (peacemove.has_xoff() == true && peacemove.has_yoff() == true )
		{
			bef_x_off = peacemove.xoff();
			bef_y_off = peacemove.yoff();
		}
		else
		{
			bef_x_off = 0;
			bef_y_off = 0;
		}

		int nxt_x_off = 0, nxt_y_off = 0;

		monster->getNextOffset(bef_x_off, bef_y_off, &nxt_x_off, &nxt_y_off);

		// 더 이상 움직일 곳이 없는 경우 ㅠ.ㅠ
		if (nxt_x_off == 0 && nxt_y_off == 0)
		{

		}
		else
		{
			monster->setX(monster->getX() + nxt_x_off);
			monster->setY(monster->getY() + nxt_y_off);

			auto elist = F_Vector::getInstance()->get(monster->getX(), monster->getY());
			Scoped_Rlock(&elist->slock);
			int size = elist->size();

			// 유저가 존재합니다!! W.A.R.N.I.N.G !! W.A.R.N.I.N.G !!
			if (size > 0)
			{
				monster->setState(BATTLE);
				// 여기서부터 배틀모드가 되었다는걸 타이머에게 전송해줘요~
			}
			else
			{
				// 이동해도 되요~~ ^-^
			}
		}
	}
	
}
