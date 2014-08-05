//PMODEDEADRESPAWN

#include <string>

#include "../protobuf/userrespawn.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/setmonster.pb.h"
#include "../protobuf/init.pb.h"


#include "Completion_Port.h"
#include "DMap.h"
#include "Scoped_Lock.h"
#include "Memory_Pool.h"
#include "DMap_monster.h"
#include "msg.h"

/*
#include "Check_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"

#include "Memory_Pool.h"
#include "monster.h"
#include "DMap_monster.h"
#include "msg.h"
*/
using std::string;

void make_vector_id_in_room_except_me(Character *, vector<Character *>&, bool);
void make_vector_id_in_room(E_List *, vector<Character *>&);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);
void set_single_cast(Character *, vector<Character *>&);
void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked);


void Handler_USERRESPAWN(LPPER_IO_DATA ioInfo, string* readContents) {
	USER_RESPAWN::CONTENTS respawnmsg;
	SET_USER::CONTENTS setuserContents;
	SET_MONSTER::CONTENTS setmonsterContents;
	INIT::CONTENTS initContents;
	
	string bytestring;
	int len;
	vector<Character *> receiver, me;

	respawnmsg.ParseFromString(*readContents);

	if (ioInfo->block != nullptr)
	{
		Memory_Pool::getInstance()->pushBlock(ioInfo->block);
		ioInfo->block = nullptr;
	}
	ioInfo_Pool::getInstance()->pushBlock(ioInfo);

	auto FVEC = F_Vector::getInstance();

	int ID = respawnmsg.id();
	int x = respawnmsg.x();
	int y = respawnmsg.y();

	auto AMAP = Access_Map::getInstance();

	Character* myChar = nullptr;
	{
		myChar = AMAP->find(ID);

		if (myChar == nullptr) {
			puts("유저 나감 ㅡ.ㅡ");
			// 죽었다고 나간 노매너 유저임. 흥!!
			return;
		}
	}
	
	me.push_back(myChar);
	// 힐!! 체력 만빵!!
	myChar->setHPMax();
	myChar->setX(x);
	myChar->setY(y);
	int lv = myChar->getLv();
	int prtHp = myChar->getPrtHp();
	int maxHp = myChar->getMaxHp();
	int power = myChar->getPower();
	int maxexp = myChar->getMaxExp();
	int prtExp = myChar->getPrtExp();
	
	E_List* elist = FVEC->get(x, y);

	//내 캐릭을 방에 추가해요!!

	// x와 y의 초기값을 가져온다.   
	{
		auto myData = initContents.mutable_data()->Add();
		myData->set_id(ID);
		myData->set_x(x);
		myData->set_y(y);
		myData->set_lv(lv);
		myData->set_maxhp(maxHp);
		myData->set_power(power);
		myData->set_maxexp(maxexp);
	}

	initContents.SerializeToString(&bytestring);
	len = bytestring.length();

	{
		//		Scoped_Rlock SR(&elist->slock);
		send_message(msg(PINIT, len, bytestring.c_str()), me, true);
	}
	receiver.clear();
	bytestring.clear();
	initContents.clear_data();

	{
		//		Scoped_Wlock SW(&elist->slock);
		elist->push_back(myChar);
	}

	// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.
	{
		auto myData = setuserContents.mutable_data()->Add();
		myData->set_id(ID);
		myData->set_x(x);
		myData->set_y(y);
		myData->set_lv(lv);
		myData->set_prthp(prtHp);
		myData->set_maxhp(maxHp);
		myData->set_power(power);
		myData->set_prtexp(maxexp);
	}

	setuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	{
		//		Scoped_Rlock SR(&elist->slock);

		// 내가 있는 방에 있는 친구들에게 내가 등장함을 알린다.
		// 나와 같은방에 있는 친구들은 누구?
		make_vector_id_in_room_except_me(myChar, receiver, false/*autolock*/);

		send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, false);
		// 이제 생성한 char에 대해서 자료구조에 넣어주었고 내가등장함을 다른 유저에게 알렸다. 이제부턴 char 에대해서 lock을 해줘야 겠다.
		// 근데 해줄 곳이 없네.. 캐릭터를 read write 하는 곳에 해야하는데 그런 곳이 없으니. 내 판단 맞나요?
		// RE : 굿굿!!

		setuserContents.clear_data();
		bytestring.clear();

		// 리스폰된 유저와 같은 방에있는 유저들의 정보를 전송한다.
		for (int i = 0; i < receiver.size(); i++) {
			auto tmpChar = receiver[i];

			auto tempData = setuserContents.mutable_data()->Add();
			tempData->set_id(tmpChar->getID());
			tempData->set_x(tmpChar->getX());
			tempData->set_y(tmpChar->getY());
			tempData->set_lv(lv);
			tempData->set_prthp(tmpChar->getPrtHp());
			tempData->set_maxhp(maxHp);
			tempData->set_power(power);
			tempData->set_prtexp(prtExp);

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

	auto FVEC_M = F_Vector_Mon::getInstance();
	vector<Monster *> vec_mon;
	//같은방에 있는 몬스터의 정보를 전송한다.
	E_List_Mon* elist_m = FVEC_M->get(x, y);
	{
		//		Scoped_Rlock SR(&elist_m->slock);
		make_monster_vector_in_room(myChar, vec_mon, false);
		for (int i = 0; i < vec_mon.size(); ++i)
		{
			Monster* tmpMon = vec_mon[i];
			{
				//				Scoped_Wlock(tmpMon->getLock());
				tmpMon->SET_BATTLE_MODE();
			}
			auto setmon = setmonsterContents.add_data();
			setmon->set_id(tmpMon->getID());
			setmon->set_x(tmpMon->getX());
			setmon->set_y(tmpMon->getY());
			setmon->set_name(tmpMon->getName());
			setmon->set_lv(tmpMon->getLv());
			setmon->set_prthp(tmpMon->getPrtHp());
			setmon->set_maxhp(tmpMon->getMaxHp());
			setmon->set_power(tmpMon->getPower());

			if (setmonsterContents.data_size() == SET_MONSTER_MAXIMUM)
			{
				setmonsterContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PSET_MON, len, bytestring.c_str()), me, false);
				setmonsterContents.clear_data();
				bytestring.clear();
			}
		}
		setmonsterContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_MON, len, bytestring.c_str()), me, false);

		vec_mon.clear();
		setmonsterContents.clear_data();
		bytestring.clear();
	}
}