#include <vector>

#include "../protobuf/connect.pb.h"
#include "../protobuf/disconn.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"

#include "Check_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "msg.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"

using namespace std;

void printLog(const char *msg, ...);
void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents);
void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void set_single_cast(Character*, vector<Character *>&);
void make_vector_id_in_room_except_me(Character*, vector<Character *>&, bool);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);

bool Boundary_Check(int, const int,const int, int, int);


void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents)
{
	//Client_Map *CMap = Client_Map::getInstance();
	Sock_set *sock_set = Sock_set::getInstance();
	auto FVEC = F_Vector::getInstance();
	auto AMAP = Access_Map::getInstance();
	auto CMap = Check_Map::getInstance();

	CONNECT::CONTENTS connect;
	INIT::CONTENTS initContents;
	SET_USER::CONTENTS setuserContents;
	string bytestring;
	int len;
	vector<Character *> receiver;

	connect.ParseFromString(*readContents);
	if (connect.data() != "HELLO SERVER!")
	{
		//가짜 클라이언트
	}
	int char_id;
	int x, y;
	static int id = 0;

	// 캐릭터 객체를 생성 후
	char_id = InterlockedIncrement((unsigned *)&id);
	Character* c = new Character(char_id);
	c->setSock(handleInfo->hClntSock);
	
	x = c->getX();
	y = c->getY();
	ioInfo->id = char_id;
	ioInfo->myCharacter = c;
	
	E_List* elist = FVEC->get(x, y);

	{
		Scoped_Wlock(&AMAP->slock);
		Scoped_Wlock(&elist->slock);
		AMAP->insert(char_id, c);
		CMap->insert(handleInfo->hClntSock, char_id);
		sock_set->erase(handleInfo->hClntSock);
		elist->push_back(c);
	}

	// x와 y의 초기값을 가져온다.   
	initContents.clear_data();

	{
		auto myData = initContents.mutable_data()->Add();
		myData->set_id(char_id);
		myData->set_x(x);
		myData->set_y(y);
	}

	bytestring.clear();
	initContents.SerializeToString(&bytestring);
	len = bytestring.length();

	set_single_cast(c, receiver);
	{
		Scoped_Rlock(&elist->slock);
		send_message(msg(PINIT, len, bytestring.c_str()), receiver, true);
	}
	receiver.clear();

	// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.
	setuserContents.clear_data();

	{
		auto myData = setuserContents.mutable_data()->Add();
		myData->set_id(char_id);
		myData->set_x(x);
		myData->set_y(y);
	}

	bytestring.clear();
	setuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	{
		Scoped_Rlock(&elist->slock);

		make_vector_id_in_room_except_me(c, receiver, false/*autolock*/);
		send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, false);
//		receiver.clear();

		// PCONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.
		// 나와 같은방에 있는 친구들은 누구?
		setuserContents.clear_data();

		for (int i = 0; i < receiver.size(); i++) {
			auto tmpChar = receiver[i];

			auto tempData = setuserContents.mutable_data()->Add();
			tempData->set_id(tmpChar->getID());
			tempData->set_x(tmpChar->getX());
			tempData->set_y(tmpChar->getY());

			if (setuserContents.data_size() == SET_USER_MAXIMUM) // SET_USER_MAXIMUM이 한계치로 접근하려고 할 때
			{
				bytestring.clear();
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				set_single_cast(c, receiver);
				send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, false);
				receiver.clear();

				setuserContents.clear_data();
			}
		}
		bytestring.clear();
		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();
		receiver.clear();

		set_single_cast(c, receiver);
		{
			Scoped_Rlock(&elist->slock);
			send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);
		}
		receiver.clear();
	}
}

void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents)
{
	MOVE_USER::CONTENTS moveuserContents;
	ERASE_USER::CONTENTS eraseuserContents;
	SET_USER::CONTENTS setuserContents;
	std::string bytestring;

	auto Amap = Access_Map::getInstance();
	auto FVEC = F_Vector::getInstance();

	int x = pCharacter->getX(), y = pCharacter->getY();
	E_List* elist = FVEC->get(x, y);

	vector<Character*> charId_in_room_except_me;
	
	moveuserContents.ParseFromString(*readContents);

	int cur_id, x_off, y_off;
	int len;

		vector<Character *> me;
		me.push_back(pCharacter);

		auto user = moveuserContents.data(0);
		cur_id = user.id();
		x_off = user.xoff();
		y_off = user.yoff();

		/* 경계값 체크 로직 */
		if (Boundary_Check(cur_id, x, y, x_off, y_off) == false) {
			return;
		}

		// 기존의 방의 유저들의 정보를 삭제함

		// 나와 같은방에 있는 친구들은 누구?
		{
			Scoped_Rlock(&elist->slock);
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

			send_message(msg(PERASE_USER, len, bytestring.c_str()), me, true);

			bytestring.clear();
			eraseuserContents.clear_data();

			auto eraseuser = eraseuserContents.add_data();
			eraseuser->set_id(cur_id);
			eraseuserContents.SerializeToString(&bytestring);
			len = bytestring.length();
		
			send_message(msg(PERASE_USER, len, bytestring.c_str()), charId_in_room_except_me, true);
		}


		// 기존 방의 유저들에게 내가 사라짐을 알림
		
		bytestring.clear();
		eraseuserContents.clear_data();

		charId_in_room_except_me.clear();

		// 캐릭터를 해당 좌표만큼 이동시킴
		{
			Scoped_Wlock(&elist->slock);
			elist->erase(cur_id);
		}
		int newX = x + x_off, newY = y + y_off;
		pCharacter->setX(newX);
		pCharacter->setY(newY);

		elist = FVEC->get(newX, newY);
		{
			Scoped_Wlock(&elist->slock);
			elist->push_back(pCharacter);
		}

		// 나와 같은방에 있는 친구들은 누구?
		{
			Scoped_Rlock(&elist->slock);
			make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

			// 새로운 방의 유저들에게 내가 등장함을 알림
			x = pCharacter->getX(), y = pCharacter->getY();
			auto setuser = setuserContents.add_data();
			setuser->set_id(cur_id);
			setuser->set_x(x);
			setuser->set_y(y);

			setuserContents.SerializeToString(&bytestring);
			len = bytestring.length();
			
			send_message(msg(PSET_USER, len, bytestring.c_str()), charId_in_room_except_me, false);
			
			bytestring.clear();
			setuserContents.clear_data();


			//setuser에 나를 추가함.
			{
				auto setuser = setuserContents.add_data();
				setuser->set_id(cur_id);
				setuser->set_x(x);
				setuser->set_y(y);
			}

			for (int i = 0; i < charId_in_room_except_me.size(); ++i)
			{
				auto setuser = setuserContents.add_data();
				auto tmpChar = charId_in_room_except_me[i];
				setuser->set_id(tmpChar->getID());
				setuser->set_x(tmpChar->getX());
				setuser->set_y(tmpChar->getY());

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
		}

		bytestring.clear();
		setuserContents.clear_data();

		charId_in_room_except_me.clear();

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
