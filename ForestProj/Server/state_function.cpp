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
		//��¥ Ŭ���̾�Ʈ
	}
	int char_id;
	int x, y;
	static int id = 0;

	// ĳ���� ��ü�� ���� ��
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

	set_single_cast(c, receiver);
	{
		Scoped_Rlock(&elist->slock);
		send_message(msg(PINIT, len, bytestring.c_str()), receiver, true);
	}
	receiver.clear();

	// ���� ������ ĳ������ ������ �ٸ� ������ �����鿡�� �����Ѵ�.
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

		// PCONNECT�� ������ �������� �ٸ� ��ü���� ������ �����Ѵ�.
		// ���� �����濡 �ִ� ģ������ ����?
		setuserContents.clear_data();

		for (int i = 0; i < receiver.size(); i++) {
			auto tmpChar = receiver[i];

			auto tempData = setuserContents.mutable_data()->Add();
			tempData->set_id(tmpChar->getID());
			tempData->set_x(tmpChar->getX());
			tempData->set_y(tmpChar->getY());

			if (setuserContents.data_size() == SET_USER_MAXIMUM) // SET_USER_MAXIMUM�� �Ѱ�ġ�� �����Ϸ��� �� ��
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

		/* ��谪 üũ ���� */
		if (Boundary_Check(cur_id, x, y, x_off, y_off) == false) {
			return;
		}

		// ������ ���� �������� ������ ������

		// ���� �����濡 �ִ� ģ������ ����?
		{
			Scoped_Rlock(&elist->slock);
			make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

			for (int i = 0; i < charId_in_room_except_me.size(); ++i)
			{
				auto eraseuser = eraseuserContents.add_data();
				eraseuser->set_id(charId_in_room_except_me[i]->getID());

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

			auto eraseuser = eraseuserContents.add_data();
			eraseuser->set_id(cur_id);
			eraseuserContents.SerializeToString(&bytestring);
			len = bytestring.length();
		
			send_message(msg(PERASE_USER, len, bytestring.c_str()), charId_in_room_except_me, true);
		}


		// ���� ���� �����鿡�� ���� ������� �˸�
		
		bytestring.clear();
		eraseuserContents.clear_data();

		charId_in_room_except_me.clear();

		// ĳ���͸� �ش� ��ǥ��ŭ �̵���Ŵ
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

		// ���� �����濡 �ִ� ģ������ ����?
		{
			Scoped_Rlock(&elist->slock);
			make_vector_id_in_room_except_me(pCharacter, charId_in_room_except_me, false/*autolock*/);

			// ���ο� ���� �����鿡�� ���� �������� �˸�
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


			//setuser�� ���� �߰���.
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
		//��¥ Ŭ���̾�Ʈ
	}

	printLog("Nomal turn off\n");
	remove_valid_client(handleInfo, ioInfo);
}
