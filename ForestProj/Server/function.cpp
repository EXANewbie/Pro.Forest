#include <WinSock2.h>
#include <vector>

#include "../protobuf/eraseuser.pb.h"

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
				// 큐에 들어감 ^.^
			}
			else
			{
				// 너에겐 수많은 이유가 있겠지... 하지만 아마도 그 수많은 이유들의 공통점은 소켓에 전송할 수 없는 것이 아닐까?
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

void closeClient(SOCKET sock, int id)
{
	Client_Map *CMap = Client_Map::getInstance();
	vector<SOCKET> send_list;

	int ret = closesocket(sock);

	if (ret != WSAENOTSOCK)
	{
		// 처음으로 소켓을 닫을 때.
		set_multicast_in_room_except_me(id, send_list, false/*not autolock*/);

		CMap->erase(id);

		ERASE_USER::CONTENTS contents;
		contents.add_data()->set_id(id);

		std::string bytestring;
		contents.SerializeToString(&bytestring);
		send_message(msg(PERASE_USER, sizeof(int), bytestring.c_str()), send_list);
	}
	else
	{
		//이미 삭제된 소켓.
	}

}

void remove_valid_client(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo)
{
	Client_Map *CMap = Client_Map::getInstance();

	if (ioInfo->id == NOT_JOINED) // 현재 유저가 PCONNECT를 보내지 않은 상태일 경우
	{
		closesocket(handleInfo->hClntSock);
		free(handleInfo); free(ioInfo);
		return;
	}

	CMap->lock();
	int char_id = CMap->find_sock_to_id(handleInfo->hClntSock);
	// 아이디가 비어있는 경우
	if (char_id == -1 || char_id != ioInfo->id)
	{
		// 이미 삭제 처리 된 경우를 여기에 명시한다.
	}
	else
	{
		printf("sock : %d char_id : %d\n", handleInfo, char_id);
		closeClient(handleInfo->hClntSock, ioInfo->id);
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