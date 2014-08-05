#include <cstdio>
#include <string>
#include <vector>

#include "Completion_Port.h"
#include "types.h"
#include "Client_Map.h"
#include "msg.h"
#include "Sock_set.h"
#include "Memory_Pool.h"

#include <Windows.h>

using std::vector;
using std::string;

void printLog(const char *msg, ...);
void set_single_cast(Character *, vector<Character *>&);
void set_multicast_in_room_except_me(Character*, vector<int>&, bool);
void send_message(msg, vector<int> &,bool);
void unpack(msg, char *, int *);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);
void Handler_PCONNECT(LPPER_HANDLE_DATA, LPPER_IO_DATA, std::string*);
void Handler_PMOVE_USER(Character *, std::string*);
void Handler_PDISCONN(LPPER_HANDLE_DATA, LPPER_IO_DATA, std::string*);
void Handler_PUSER_ATTCK(Character *, std::string*);

void Handler_HELLOWORLD(LPPER_IO_DATA, string*);
void Handler_PEACEMOVE(LPPER_IO_DATA, string*);
void Handler_BATTLEATTACK(LPPER_IO_DATA, string*);
void Handler_DEADRESPAWN(LPPER_IO_DATA, string*);
void Handler_USERRESPAWN(LPPER_IO_DATA, string*);

int k;

unsigned WINAPI Server_Worker(LPVOID pComPort)
{
	Sock_set *sock_set = Sock_set::getInstance();
	
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();

	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	while (true)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);

		if (ioInfo->RWmode == READ)
		{
			sock = handleInfo->hClntSock;
			printLog("MESSAGE RECEIVED!");
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printLog("@Abnormal turn off ");
				remove_valid_client(handleInfo, ioInfo);
				continue;
			}

			// 헤더가 들어온 경우
			if (ioInfo->type == UNDEFINED)
			{
				printLog("Input Header\n");
				int *param[] = { &(ioInfo->type), &(ioInfo->len) };
				copy_to_param(param, 2, ioInfo->buffer);

				ioInfo->offset = 0;
				ioInfo->block = MemoryPool->popBlock();
				ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
				ioInfo->wsaBuf.len = ioInfo->len;
				memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
				flags = 0;

				int ret = WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);

				if (ret == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSA_IO_PENDING)
					{
					}
					else
					{
						// 소켓 에러 발생
					}
				}
			}
			// 바디 부분이 들어온 경우
			else {
				// 아직 메시지가 완전히 들어오지 않은 경우
				if (ioInfo->offset + bytesTrans < ioInfo->len)
				{
					printLog("Input Middle(%d + %d) , goal(%d) \n",ioInfo->offset, bytesTrans,ioInfo->len);
					ioInfo->offset += bytesTrans;
					ioInfo->wsaBuf.buf = ioInfo->block->getBuffer()+ioInfo->offset;
					ioInfo->wsaBuf.len = ioInfo->len - ioInfo->offset;
					memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
					flags = 0;

					int ret = WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);

					if (ret == SOCKET_ERROR)
					{
						if (WSAGetLastError() == WSA_IO_PENDING)
						{
						}
						else
						{
							// 소켓 에러 발생
						}
					}
				}
				// 완전히 메시지가 다 들어온 경우
				else {
					printLog("Input End Of Body(%d->%d)\n", ioInfo->offset, bytesTrans);
					int type, len;
					type = ioInfo->type;
					len = ioInfo->len;
					
					std::string readContents(ioInfo->wsaBuf.buf, len);

					if (type == PDISCONN) // 정상적인 종료의 경우
					{
						Handler_PDISCONN(handleInfo, ioInfo, &readContents);
						continue;
					}
					else if (type == PCONNECT) // 새로 들어온 경우
					{
						Handler_PCONNECT(handleInfo, ioInfo, &readContents);
					}
					else if (type == PMOVE_USER)// 유저가 이동하는 경우
					{
						Handler_PMOVE_USER(ioInfo->myCharacter, &readContents);
					}
					else if (type == PUSER_ATTCK)
					{
						// 유저로부터 유저가 공격할 것이라고 패킷을 보냈다.
						// 데미지 계산을 하고 그 데미지로 몬스터 체력을 깎고 클라한테 그 결과를 보낼 것이다.

						Handler_PUSER_ATTCK(ioInfo->myCharacter, &readContents);

					}

					memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
					ioInfo->wsaBuf.len = HEADER_SIZE;
					ioInfo->wsaBuf.buf = ioInfo->buffer;
					ioInfo->RWmode = READ;
					ioInfo->type = UNDEFINED;
					ioInfo->len = UNDEFINED;
					ioInfo->offset = UNDEFINED;

					if (ioInfo->block != nullptr) {
						MemoryPool->pushBlock(ioInfo->block);
						ioInfo->block = nullptr;
					}
					flags = 0;

					int ret = WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);

					if (ret == SOCKET_ERROR)
					{
						if (WSAGetLastError() == WSA_IO_PENDING)
						{
						}
						else
						{
							// 소켓 에러 발생
						}
					}
				}
			}
		}
		else if( ioInfo->RWmode == WRITE )// WRITE
		{
			sock = handleInfo->hClntSock;
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printLog("나 출력되는거 맞음?ㅋ\n");
				Sleep(10000);
				remove_valid_client(handleInfo, ioInfo);
				continue;
			}

			printLog("MESSAGE SEND!");

			if (ioInfo->block != nullptr) {
				MemoryPool->pushBlock(ioInfo->block);
				ioInfo->block = nullptr;
			}
			ioInfoPool->pushBlock(ioInfo);
			printLog("k Decrement %d\n", InterlockedDecrement((unsigned int *)&k));
		}
		else // TIMER
		{
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printLog("@Abnormal turn off ");
				ioInfoPool->pushBlock(ioInfo);
				continue;
			}

			int type, len;
			type = ioInfo->type;
			len = ioInfo->len;
	
			printLog("AI Timer(%d)\n", ioInfo->type);

			std::string readContents(ioInfo->wsaBuf.buf, len);
			if (ioInfo->type == PHELLOWORLD)
			{
				Handler_HELLOWORLD(ioInfo, &readContents);
			}
			else if (ioInfo->type == PMODEPEACEMOVE)
			{
				Handler_PEACEMOVE(ioInfo, &readContents);
			}
			else if (ioInfo->type == PMODEBATTLEATTACK)
			{
				Handler_BATTLEATTACK(ioInfo, &readContents);
			}
			else if ( ioInfo->type == PMODEDEADRESPAWN)
			{
				Handler_DEADRESPAWN(ioInfo, &readContents);
			}
			else if (ioInfo->type == USERRESPAWN)
			{
				Handler_USERRESPAWN(ioInfo, &readContents);
			}
		}
	}

	return 0;
}
