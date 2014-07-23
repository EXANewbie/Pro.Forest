#include <WinSock2.h>
#include <vector>

#include "../protobuf/eraseuser.pb.h"

#include "Client_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "msg.h"
#include "character.h"
#include "Sock_set.h"
#include "Memory_Pool.h"

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
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();
	Client_Map *CMap = Client_Map::getInstance();
	//vector< pair<int,SOCKET> > errors;

	int len;

	for (int i = 0; i < send_list.size(); i++)
	{
		int id = send_list[i];
		if (autolocked == true)
		{
			CMap->lock();
		}
		SOCKET sock = CMap->find_id_to_sock(id);
		
		if (sock != SOCKET_ERROR) {
			LPPER_IO_DATA ioInfo = ioInfoPool->popBlock();
			ioInfo->block = MemoryPool->popBlock();

			unpack(message, ioInfo->block->getBuffer(), &len);
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = len;
			ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
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
					if (ioInfo->block != nullptr) {
						MemoryPool->pushBlock(ioInfo->block);
					}
					ioInfoPool->pushBlock(ioInfo);
					//free(ioInfo);
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
		// 처음으로 소켓을 닫을 때.
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
		//이미 삭제된 소켓.
	}

}

void remove_valid_client(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo)
{
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto HandlerPool = Handler_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();
	Client_Map *CMap = Client_Map::getInstance();

	if (ioInfo->id == NOT_JOINED) // 현재 유저가 PCONNECT를 보내지 않은 상태일 경우
	{
		closesocket(handleInfo->hClntSock);

		HandlerPool->pushBlock(handleInfo);
		if (ioInfo->block != nullptr) {
			MemoryPool->pushBlock(ioInfo->block);
		}
		ioInfoPool->pushBlock(ioInfo);
//		free(handleInfo); free(ioInfo);
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
		printf("sock : %d char_id : %d\n", handleInfo->hClntSock, char_id);
		closeClient(handleInfo->hClntSock, ioInfo->id,ioInfo->myCharacter);

		HandlerPool->pushBlock(handleInfo);
		ioInfoPool->pushBlock(ioInfo);
		//		free(handleInfo); free(ioInfo);
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