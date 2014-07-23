#include <WinSock2.h>
#include <vector>

#include "../protobuf/eraseuser.pb.h"

#include "Client_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "msg.h"
#include "character.h"
#include "Sock_set.h"

using namespace std;

extern int k;

void set_single_cast(int, vector<int>&);
void make_vector_id_in_room_except_me(Character*, vector<int>&, bool);
void send_message(msg, vector<int> &,bool);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);

void set_single_cast(int id, vector<int>& send_list)
{
	send_list.push_back(id);
}

void make_vector_id_in_room_except_me(Character* myChar, vector<int>& send_list, bool autolocked)
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
			LPPER_IO_DATA ioInfo = new PER_IO_DATA;

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
		make_vector_id_in_room_except_me(myChar, send_list, false/*not autolock*/);

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