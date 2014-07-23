#include <vector>

#include "../protobuf/connect.pb.h"
#include "../protobuf/disconn.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"

#include "Client_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "msg.h"
#include "character.h"
#include "Sock_set.h"

using namespace std;

void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents);
void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void set_single_cast(int, vector<int>&);
void set_multicast_in_room_except_me(Character*, vector<int>&, bool);
void send_message(msg, vector<int> &, bool);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);


void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents)
{
	Client_Map *CMap = Client_Map::getInstance();
	Sock_set *sock_set = Sock_set::getInstance();

	CONNECT::CONTENTS connect;
	INIT::CONTENTS initContents;
	SET_USER::CONTENTS setuserContents;
	string bytestring;
	int len;
	vector<int> receiver;

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
	x = c->getX();
	y = c->getY();
	ioInfo->id = char_id;
	ioInfo->myCharacter = c;

	bool isValid = false;
	while (true)
	{
		CMap->lock();
		isValid = CMap->insert(char_id, handleInfo->hClntSock, c);
		CMap->unlock();
		if (isValid == true)
		{
			sock_set->erase(handleInfo->hClntSock);
			break;
		}
		else
		{
			SleepEx(100, true);
		}
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

	set_single_cast(char_id, receiver);
	send_message(msg(PINIT, len, bytestring.c_str()), receiver, true);
	receiver.clear();

	// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.
	setuserContents.clear_data();// .clear_data();
	{
		auto myData = setuserContents.mutable_data()->Add();
		myData->set_id(char_id);
		myData->set_x(x);
		myData->set_y(y);
	}

	bytestring.clear();
	setuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	set_multicast_in_room_except_me(c, receiver, true/*autolock*/);
	send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);
	receiver.clear();


	// PCONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.
	// 나와 같은방에 있는 친구들은 누구?
	setuserContents.clear_data();

	CMap->lock();
	for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
	{
		if (itr->second == c) // 캐릭터 맵에 현재 들어간 내 객체의 정보를 보내려 할 때는 건너뛴다.
			continue;

		int tID = itr->second->getID();
		int tx = itr->second->getX();
		int ty = itr->second->getY();


		if (tx == x && ty == y)
		{
			auto tempData = setuserContents.mutable_data()->Add();
			tempData->set_id(tID);
			tempData->set_x(tx);
			tempData->set_y(ty);
		}
	}
	CMap->unlock();

	bytestring.clear();
	setuserContents.SerializeToString(&bytestring);
	len = bytestring.length();

	set_single_cast(char_id, receiver);
	send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);
	receiver.clear();
}

void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents)
{
	Client_Map *CMap = Client_Map::getInstance();

	MOVE_USER::CONTENTS moveuserContents;
	ERASE_USER::CONTENTS eraseuserContents;
	SET_USER::CONTENTS setuserContents;
	std::string bytestring;
	vector<int> receiver;

	moveuserContents.ParseFromString(*readContents);

	int cur_id, x_off, y_off;
	int len;

	for (int i = 0; i < moveuserContents.data_size(); ++i)
	{
		auto user = moveuserContents.data(i);
		cur_id = user.id();
		x_off = user.xoff();
		y_off = user.yoff();

		// 기존의 방의 유저들의 정보를 삭제함
		// 나와 같은방에 있는 친구들은 누구?

		set_multicast_in_room_except_me(pCharacter, receiver, true/*autolock*/);
		CMap->lock();
		for (auto iter = CMap->begin(); iter != CMap->end(); iter++)
		{
			if (iter->second->getX() == pCharacter->getX() && iter->second->getY() == pCharacter->getY())
			{
				int nid = iter->second->getID();

				if (nid == pCharacter->getID())
					continue;

				auto eraseuser = eraseuserContents.add_data();
				eraseuser->set_id(nid);
			}
		}
		CMap->unlock();

		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		set_single_cast(pCharacter->getID(), receiver);
		send_message(msg(PERASE_USER, len, bytestring.c_str()), receiver, true);

		receiver.clear();
		bytestring.clear();
		eraseuserContents.clear_data();

		// 기존 방의 유저들에게 내가 사라짐을 알림
		auto eraseuser = eraseuserContents.add_data();
		eraseuser->set_id(cur_id);
		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		set_multicast_in_room_except_me(pCharacter, receiver, true/*autolock*/);
		send_message(msg(PERASE_USER, len, bytestring.c_str()), receiver, true);

		receiver.clear();
		bytestring.clear();
		eraseuserContents.clear_data();

		// 캐릭터를 해당 좌표만큼 이동시킴
		pCharacter->setX(pCharacter->getX() + x_off);
		pCharacter->setY(pCharacter->getY() + y_off);

		// 새로운 방의 유저들에게 내가 등장함을 알림
		int id = pCharacter->getID(), x = pCharacter->getX(), y = pCharacter->getY();
		auto setuser = setuserContents.add_data();
		setuser->set_id(id);
		setuser->set_x(x);
		setuser->set_y(y);

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		set_multicast_in_room_except_me(pCharacter, receiver, true/*autolock*/);
		send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);

		receiver.clear();
		bytestring.clear();
		setuserContents.clear_data();

		// 새로운 방의 유저들의 정보를 불러옴

		CMap->lock();
		for (auto iter = CMap->begin(); iter != CMap->end(); iter++)
		{
			if (iter->second->getX() == x && iter->second->getY() == y)
			{
				int nid = iter->second->getID();
				int nx = iter->second->getX();
				int ny = iter->second->getY();

				auto setuser = setuserContents.add_data();
				setuser->set_id(nid);
				setuser->set_x(nx);
				setuser->set_y(ny);
			}
		}
		CMap->unlock();

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		set_single_cast(pCharacter->getID(), receiver);
		send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);

		receiver.clear();
		bytestring.clear();
		setuserContents.clear_data();

		printf("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);
	}
}

void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents)
{
	DISCONN::CONTENTS disconn;

	disconn.ParseFromString(*readContents);
	if (disconn.data() != "BYE SERVER!")
	{
		//가짜 클라이언트
	}

	printf("Nomal turn off\n");
	remove_valid_client(handleInfo, ioInfo);
}
