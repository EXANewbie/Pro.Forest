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

void printLog(const char *msg, ...);
void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents);
void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void set_single_cast(int, vector<int>&);
void make_vector_id_in_room_except_me(Character*, vector<int>&, bool);
void send_message(msg, vector<int> &, bool);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);

bool Boundary_Check(int, const int,const int, int, int);


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
		//��¥ Ŭ���̾�Ʈ
	}
	int char_id;
	int x, y;
	static int id = 0;

	// ĳ���� ��ü�� ���� ��
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

	// x�� y�� �ʱⰪ�� �����´�.   
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

	// ���� ������ ĳ������ ������ �ٸ� ������ �����鿡�� �����Ѵ�.
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

	make_vector_id_in_room_except_me(c, receiver, true/*autolock*/);
	send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);
	receiver.clear();


	// PCONNECT�� ������ �������� �ٸ� ��ü���� ������ �����Ѵ�.
	// ���� �����濡 �ִ� ģ������ ����?
	setuserContents.clear_data();

	CMap->lock();
	for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
	{
		if (itr->second == c) // ĳ���� �ʿ� ���� �� �� ��ü�� ������ ������ �� ���� �ǳʶڴ�.
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

			if (setuserContents.data_size() == SET_USER_MAXIMUM) // SET_USER_MAXIMUM�� �Ѱ�ġ�� �����Ϸ��� �� ��
			{
				bytestring.clear();
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				set_single_cast(char_id, receiver);
				send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, false);
				receiver.clear();

				setuserContents.clear_data();
			}
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

	vector<int> charId_in_room_except_me;
	
	moveuserContents.ParseFromString(*readContents);

	int cur_id, x_off, y_off;
	int len;

	for (int i = 0; i < moveuserContents.data_size(); ++i)
	{
		vector<int> me;
		
		auto user = moveuserContents.data(i);
		cur_id = user.id();
		x_off = user.xoff();
		y_off = user.yoff();

		me.clear();
		me.push_back(cur_id);

		CMap->lock();
		Character *cCharacter = CMap->find_id_to_char(cur_id);

		if (cCharacter == nullptr) {
			printLog("character(%d) is not exist\n", cur_id);
			continue;
		}
		int cX = cCharacter->getX(), cY = cCharacter->getY();
		CMap->unlock();

		/* ��谪 üũ ���� */
		if (Boundary_Check(cur_id, cX, cY, x_off, y_off) == false) {
			continue;
		}

		// ������ ���� �������� ������ ������

		// ���� �����濡 �ִ� ģ������ ����?
		make_vector_id_in_room_except_me(cCharacter, charId_in_room_except_me, true/*autolock*/);
		
		for (int i = 0; i < charId_in_room_except_me.size(); ++i)
		{
			auto eraseuser = eraseuserContents.add_data();
			eraseuser->set_id(charId_in_room_except_me[i]);

			if (eraseuserContents.data_size() == ERASE_USER_MAXIMUM) // ERASE_USER_MAXIMUM�� �Ѱ�ġ�� �����Ϸ��� �� ��
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

		// ���� ���� �����鿡�� ���� ������� �˸�
		auto eraseuser = eraseuserContents.add_data();
		eraseuser->set_id(cur_id);
		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PERASE_USER, len, bytestring.c_str()), charId_in_room_except_me, true);

		bytestring.clear();
		eraseuserContents.clear_data();

		charId_in_room_except_me.clear();

		// ĳ���͸� �ش� ��ǥ��ŭ �̵���Ŵ
		cCharacter->setX(cCharacter->getX() + x_off);
		cCharacter->setY(cCharacter->getY() + y_off);

		// ���� �����濡 �ִ� ģ������ ����?
		make_vector_id_in_room_except_me(cCharacter, charId_in_room_except_me, true/*autolock*/);

		// ���ο� ���� �����鿡�� ���� �������� �˸�
		int x = cCharacter->getX(), y = cCharacter->getY();
		auto setuser = setuserContents.add_data();
		setuser->set_id(cur_id);
		setuser->set_x(x);
		setuser->set_y(y);

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), charId_in_room_except_me, true);

		bytestring.clear();
		setuserContents.clear_data();
		

		//setuser�� ���� �߰���.
		{
			auto setuser = setuserContents.add_data();
			setuser->set_id(cur_id);
			setuser->set_x(x);
			setuser->set_y(y);
		}
		
		for (int i = 0; i < charId_in_room_except_me.size(); ++i)
		{
			int charid = charId_in_room_except_me[i];
			auto setuser = setuserContents.add_data();
			setuser->set_id(charid);
			setuser->set_x(CMap->find_id_to_char(charid)->getX());
			setuser->set_y(CMap->find_id_to_char(charid)->getY());

			if (setuserContents.data_size() == SET_USER_MAXIMUM) {
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				send_message(msg(PSET_USER, len, bytestring.c_str()), charId_in_room_except_me, true);

				setuserContents.clear_data();
				bytestring.clear();
			}
		}

		setuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		send_message(msg(PSET_USER, len, bytestring.c_str()), me, true);

		bytestring.clear();
		setuserContents.clear_data();

		charId_in_room_except_me.clear();

		//printf("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);
		printLog("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);
	}
}

void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents)
{
	DISCONN::CONTENTS disconn;

	disconn.ParseFromString(*readContents);
	if (disconn.data() != "BYE SERVER!")
	{
		//��¥ Ŭ���̾�Ʈ
	}

	printLog("Nomal turn off\n");
	remove_valid_client(handleInfo, ioInfo);
}
