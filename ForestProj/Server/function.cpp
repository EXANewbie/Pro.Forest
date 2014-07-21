#include <WinSock2.h>
#include <vector>

#include "Client_Map.h"
#include "Disc_user_map.h"
#include "types.h"
#include "Completion_Port.h"
#include "msg.h"

using namespace std;

extern int k;

void set_single_cast(int, vector<SOCKET>&);
void set_multicast_in_room_except_me(int, vector<SOCKET>&, bool);
void send_message(msg, vector<SOCKET> &);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(SOCKET, LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);

void set_single_cast(int id, vector<SOCKET>& send_list)
{
	Client_Map *CMap = Client_Map::getInstance();
	CMap->lock();
	SOCKET sock = CMap->find_id_to_sock(id);
	CMap->unlock();
	send_list.push_back(sock);
}

void set_multicast_in_room_except_me(int id, vector<SOCKET>& send_list, bool autolocked)
{
	Client_Map *CMap = Client_Map::getInstance();

	if (autolocked == true)
	{
		CMap->lock();
	}

	auto now = CMap->find_id_to_char(id);

	for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
	{
		if (now->getX() == itr->second.getX() && now->getY() == itr->second.getY())
		{
			if (now->getID() != itr->second.getID())
			{
				auto sock = CMap->find_id_to_sock(itr->first);
				send_list.push_back(sock);
			}
		}
	}

	if (autolocked == true)
	{
		CMap->unlock();
	}
}



void send_message(msg message, vector<SOCKET> &send_list) {
	Client_Map *CMap = Client_Map::getInstance();
	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();
	//vector< pair<int,SOCKET> > errors;

	int len;
	char buff[BUFFER_SIZE];

	unpack(message, buff, &len);

	WSABUF wsabuf;
	wsabuf.buf = buff;
	wsabuf.len = len;

	for (int i = 0; i < send_list.size(); i++)
	{
		SOCKET sock = send_list[i];
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

void closeClient(int id)
{
	Client_Map *CMap = Client_Map::getInstance();
	vector<SOCKET> send_list;

	SOCKET sock = CMap->find_id_to_sock(id);
	int ret = closesocket(sock);

	if (ret != WSAENOTSOCK)
	{
		// ó������ ������ ���� ��.
		int char_id = CMap->find_sock_to_id(sock);

		//���̵� �������� �ʴ� ����̹Ƿ�, �� ���� �����Ѵ�.
		if (char_id == -1)
		{
			return;
		}

		set_multicast_in_room_except_me(char_id, send_list, false/*not autolock*/);

		CMap->erase(id);

		send_message(msg(PERASE_USER, sizeof(int), (char*)&char_id), send_list);
	}
	else
	{
		//�̹� ������ ����.
	}

}

void remove_valid_client(SOCKET sock, LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo)
{
	Client_Map *CMap = Client_Map::getInstance();
	CMap->lock();
	int char_id = CMap->find_sock_to_id(sock);
	// ���̵� ����ִ� ���
	if (char_id == -1)
	{
		// �̹� ���� ó�� �� ��츦 ���⿡ �����Ѵ�.
	}
	else
	{
		printf("sock : %d char_id : %d\n", sock, char_id);
		closeClient(char_id);
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