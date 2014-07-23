#include <WinSock2.h>
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

extern int k;

void Handler_PCONNECT(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void Handler_PMOVE_USER(Character *pCharacter, std::string* readContents);
void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, std::string* readContents);
void set_single_cast(int, vector<int>&);
void set_multicast_in_room_except_me(Character* , vector<int>&, bool);
void send_message(msg, vector<int> &,bool);
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
		//��¥ Ŭ���̾�Ʈ
	}
	int char_id;
	int x, y;
	static int id = 0;

	// ĳ���� ��ü�� ���� ��
	int copy_id = InterlockedIncrement((unsigned *)&id);
	Character* c = new Character(copy_id);
	char_id = copy_id;
	x = c->getX();
	y = c->getY();
	ioInfo->id = copy_id;
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

	set_multicast_in_room_except_me(c, receiver, true/*autolock*/);
	send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);
	receiver.clear();

	// PCONNECT�� ������ �������� �ٸ� ��ü���� ������ �����Ѵ�.
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


		// ������ ���� �������� ������ ������
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

		// ���� ���� �����鿡�� ���� ������� �˸�
		auto eraseuser = eraseuserContents.add_data();
		eraseuser->set_id(cur_id);
		eraseuserContents.SerializeToString(&bytestring);
		len = bytestring.length();

		set_multicast_in_room_except_me(pCharacter, receiver, true/*autolock*/);
		send_message(msg(PERASE_USER, len, bytestring.c_str()), receiver, true);

		receiver.clear();
		bytestring.clear();
		eraseuserContents.clear_data();

		// ĳ���͸� �ش� ��ǥ��ŭ �̵���Ŵ
		pCharacter->setX(pCharacter->getX() + x_off);
		pCharacter->setY(pCharacter->getY() + y_off);

		// ���ο� ���� �����鿡�� ���� �������� �˸�
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

		// ���ο� ���� �������� ������ �ҷ���

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
		//��¥ Ŭ���̾�Ʈ
	}

	printf("Nomal turn off\n");
	remove_valid_client(handleInfo, ioInfo);
}


void set_single_cast(int id, vector<int>& send_list)
{
	send_list.push_back(id);
}

void set_multicast_in_room_except_me(Character* myChar, vector<int>& send_list, bool autolocked)
{
	Client_Map *CMap = Client_Map::getInstance();

	if (autolocked == true)
	{
		CMap->lock();
	}

	//Character* now = CMap->find_id_to_char(id);

	for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
	{
		if (myChar->getX() == itr->second->getX() && myChar->getY() == itr->second->getY())
		{
			if (myChar->getID() != itr->second->getID())
			{
				send_list.push_back(itr->second->getID());
			}
		}
	}

	if (autolocked == true)
	{
		CMap->unlock();
	}
}



void send_message(msg message, vector<int> &send_list, bool autolocked) {
	Client_Map *CMap = Client_Map::getInstance();
	//vector< pair<int,SOCKET> > errors;

	int len;
	char buff[BUFFER_SIZE];

	unpack(message, buff, &len);

	for (int i = 0; i < send_list.size(); i++)
	{
		int id = send_list[i];
		if (autolocked == true)
		{
			CMap->lock();
		}
		SOCKET sock = CMap->find_id_to_sock(id);
		
		if (sock != SOCKET_ERROR) {
			PER_IO_DATA *ioInfo = new PER_IO_DATA;

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			memcpy(ioInfo->buffer, buff, len);
			ioInfo->wsaBuf.len = len;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->RWmode = WRITE;

			int ret = WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

			if (ret == SOCKET_ERROR)
			{
				if (WSAGetLastError() == ERROR_IO_PENDING)
				{
					printf("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
					// ť�� �� ^.^
				}
				else
				{
					// �ʿ��� ������ ������ �ְ���... ������ �Ƹ��� �� ������ �������� �������� ���Ͽ� ������ �� ���� ���� �ƴұ�?
					free(ioInfo);
				}
				printf("Send Error (%d)\n", WSAGetLastError());
			}
			else
			{
				printf("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
			}
		}
		if (autolocked == true)
		{
			CMap->unlock();
		}
	}
}

void unpack(msg message, char *buf, int *size)
{
	int writebyte = 0;

	memcpy(buf + writebyte, &message.type, sizeof(int));
	writebyte += sizeof(int);
	memcpy(buf + writebyte, &message.len, sizeof(int));
	writebyte += sizeof(int);
	memcpy(buf + writebyte, message.buff, message.len);
	writebyte += message.len;

	*size = writebyte;
}

void closeClient(SOCKET sock, int id, Character* myChar)
{
	Client_Map *CMap = Client_Map::getInstance();
	vector<int> send_list;

	int ret = closesocket(sock);

	if (ret != WSAENOTSOCK)
	{
		// ó������ ������ ���� ��.
		set_multicast_in_room_except_me(myChar, send_list, false/*not autolock*/);

		CMap->erase(id);

		ERASE_USER::CONTENTS contents;
		contents.add_data()->set_id(id);

		std::string bytestring;
		contents.SerializeToString(&bytestring);
		send_message(msg(PERASE_USER, sizeof(int), bytestring.c_str()), send_list,false);
	}
	else
	{
		//�̹� ������ ����.
	}

}

void remove_valid_client(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo)
{
	Client_Map *CMap = Client_Map::getInstance();

	if (ioInfo->id == NOT_JOINED) // ���� ������ PCONNECT�� ������ ���� ������ ���
	{
		closesocket(handleInfo->hClntSock);
		free(handleInfo); free(ioInfo);
		return;
	}

	CMap->lock();
	int char_id = CMap->find_sock_to_id(handleInfo->hClntSock);
	// ���̵� ����ִ� ���
	if (char_id == -1 || char_id != ioInfo->id)
	{
		// �̹� ���� ó�� �� ��츦 ���⿡ ����Ѵ�.
	}
	else
	{
		printf("sock : %d char_id : %d\n", handleInfo->hClntSock, char_id);
		closeClient(handleInfo->hClntSock, ioInfo->id,ioInfo->myCharacter);
		free(handleInfo); free(ioInfo);
	}
	CMap->unlock();
}

void copy_to_buffer(char *buf, int *param[], int count)
{
	int writebyte = 0;
	for (int i = 0; i < count; i++)
	{
		memcpy(buf + writebyte, param[i], sizeof(int));
		writebyte += sizeof(int);
	}
}

void copy_to_param(int *param[], int count, char *buf)
{
	int readbyte = 0;
	for (int i = 0; i < count; i++)
	{
		memcpy(param[i], buf + readbyte, sizeof(int));
		readbyte += sizeof(int);
	}
}