#include <cstdio>
#include <string>
#include <vector>

#include "../protobuf/connect.pb.h"
#include "../protobuf/disconn.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"

#include "Completion_Port.h"
#include "types.h"
#include "Client_Map.h"
#include "msg.h"
#include "Sock_set.h"
#include "Memory_Pool.h"

#include <Windows.h>

using std::vector;
using std::string;

void set_single_cast(int, vector<int>&);
void set_multicast_in_room_except_me(Character*, vector<int>&, bool);
void send_message(msg, vector<int> &,bool);
void unpack(msg, char *, int *);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);
void Handler_PCONNECT(LPPER_HANDLE_DATA, LPPER_IO_DATA, std::string*);
void Handler_PMOVE_USER(Character *, std::string*);
void Handler_PDISCONN(LPPER_HANDLE_DATA, LPPER_IO_DATA, std::string*);

extern CRITICAL_SECTION cs;
int k;

unsigned WINAPI Server_Worker(LPVOID pComPort)
{
	Client_Map *CMap = Client_Map::getInstance();
	Sock_set *sock_set = Sock_set::getInstance();

	auto HandlerPool = Handler_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();

	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	vector<int> receiver;

	while (true)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		if (ioInfo->RWmode == READ)
		{
			puts("MESSAGE RECEIVED!");
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printf("@Abnomal turn off ");
				remove_valid_client(handleInfo, ioInfo);
				continue;
			}

			// 헤더가 들어온 경우
			if (ioInfo->type == UNDEFINED)
			{
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
					ioInfo->offset += bytesTrans;
					ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
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

					memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
					ioInfo->wsaBuf.len = HEADER_SIZE;
					ioInfo->wsaBuf.buf = ioInfo->buffer;
					ioInfo->RWmode = READ;
					if (ioInfo->block != nullptr) {
						MemoryPool->pushBlock(ioInfo->block);
					}
					ioInfo->block = nullptr;
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
		else // WRITE
		{
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printf("나 출력되는거 맞음?ㅋ\n");
				remove_valid_client(handleInfo, ioInfo);
				continue;
			}

			puts("MESSAGE SEND!");
//			free(ioInfo);
			if (ioInfo->block != nullptr) {
				MemoryPool->pushBlock(ioInfo->block);
			}
			ioInfoPool->pushBlock(ioInfo);
			printf("k Decrement %d\n", InterlockedDecrement((unsigned int *)&k));
		}
	}

	return 0;
}
