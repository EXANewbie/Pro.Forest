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
#include "Memory_Pool.h"

#include <Windows.h>

void printLog(const char *msg, ...);

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

		if (ioInfo->RWmode = READ)
		{
			printLog("MESSAGE RECEIVED!");
			if (bytesTrans == 0) // �ùٸ��� ���� ������ ���
			{
				printLog("%d : @Abnormal diconnected",sock);
				continue;
			}
			int type, len, id;
			int readByte = 0;
			memcpy(&type, ioInfo->wsaBuf.buf, sizeof(int));

			//init�� ���ؼ��� ó��. ĳ���ͼ��ϸʿ��� �־���.
			if (type == PINIT)
			{
				readByte += sizeof(int);
				memcpy(&len, ioInfo->wsaBuf.buf + readByte, sizeof(int));
				readByte += sizeof(int);
				memcpy(&id, ioInfo->wsaBuf.buf + readByte, sizeof(int));
				/*chars->lock();
				chars->insert(handleInfo->hClntSock, id);
				chars->unlock();*/
				handleInfo->char_id = id;
			}


			if (ioInfo->block != nullptr) {
				MemoryPool->pushBlock(ioInfo->block);
				ioInfo->block = nullptr;
			}
			ioInfoPool->pushBlock(ioInfo);
		}
		else
		{
			if (bytesTrans == 0) // �ùٸ��� ���� ������ ���
			{
				printLog("�� ��µǴ°� ����?��\n");
				continue;
			}

			printLog("MESSAGE SEND!");

			if (ioInfo->block != nullptr) {
				MemoryPool->pushBlock(ioInfo->block);
				ioInfo->block = nullptr;
			}
			ioInfoPool->pushBlock(ioInfo);			

		}
	}
}