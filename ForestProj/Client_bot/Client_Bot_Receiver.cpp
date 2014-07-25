#include <cstdio>
#include <string>
#include <vector>

#include "../protobuf/moveuser.pb.h"
#include "../protobuf/init.pb.h"

#include "Completion_Port.h"
#include "types.h"
#include "Memory_Pool.h"

#include <Windows.h>

void printLog(const char *msg, ...);
void copy_to_buffer(char* pBuf, int* type, int* len, std::string* content);

int k;

unsigned WINAPI Client_Bot_Worker(LPVOID pComPort)
{
	auto HandlerPool = Handler_Pool::getInstance();
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
		sock = handleInfo->hClntSock;

		if (ioInfo->RWmode == READ)
		{
			printLog("MESSAGE RECEIVED!");
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printLog("%d : @Abnormal diconnected",sock);
				continue;
			}

			int type, len;
			int readByte = 0;
			memcpy(&type, ioInfo->wsaBuf.buf, sizeof(int));
			readByte += sizeof(int);
			memcpy(&len, ioInfo->wsaBuf.buf + readByte, sizeof(int));
			readByte += sizeof(int);

			//init에 대해서만 처리.
			if (type == PINIT)
			{
				int id, x, y;
				std::string readContents(ioInfo->wsaBuf.buf + readByte, len);
				INIT::CONTENTS contents;
				contents.ParseFromString(readContents);
				
				auto user = contents.data(0);
				id = user.id();
				x = user.x();
				y = user.y();

				handleInfo->char_id = id;
			}

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			
			ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
			ioInfo->wsaBuf.len = BLOCK_SIZE;
			ioInfo->RWmode = READ;
			WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printLog("나 출력되는거 맞음?ㅋ\n");
				continue;
			}

			printLog("MESSAGE SEND!");

			if (ioInfo->block != nullptr) {
				MemoryPool->pushBlock(ioInfo->block);
				ioInfo->block = nullptr;
			}
			ioInfoPool->pushBlock(ioInfo);			
			printLog("Released!");
		}
	}
}