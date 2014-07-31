#include <WinSock2.h>
#include <vector>

#include "types.h"
#include "Completion_Port.h"
#include "Memory_Pool.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"

void copy_to_buffer(char* pBuf, int* type, int* len, std::string* content);

extern int k;
std::vector<handledata>* vec;
HANDLE TimerQueue;

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	// 해당 종료 시간이 완료 되었을 경우 해당 콜백 함수가 호출됩니다.
	auto HandleVector = vec;
	auto i = *(int *)lpParam;
	int randInt = moveRand;

	auto HandlerPool = Handler_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();

	LPPER_IO_DATA ioInfo;

	(*HandleVector)[i].tic = randInt;
	SOCKET sock = (*HandleVector)[i].handleInfo->hClntSock;

	printLog("i : %d\n", i);

	if ((*HandleVector)[i].state == PCONNECT)
	{
		(*HandleVector)[i].state = PINIT;
		int type = PCONNECT;

		std::string bytestring;
		CONNECT::CONTENTS contents;
		std::string* buff_msg = contents.mutable_data();
		*buff_msg = "HELLO SERVER!";

		contents.SerializeToString(&bytestring);
		int len = bytestring.length();

		ioInfo = ioInfoPool->popBlock();
		ioInfo->block = MemoryPool->popBlock();

		copy_to_buffer(ioInfo->block->getBuffer(), &type, &len, &bytestring);
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
		ioInfo->wsaBuf.len = len + sizeof(int)* 2;
		ioInfo->RWmode = WRITE;

		int ret = WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

		if (ret == SOCKET_ERROR)
		{
			if (WSAGetLastError() == ERROR_IO_PENDING)
			{
				printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
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
			printLog("Send Error (%d)\n", WSAGetLastError());
		}
		else
		{
			printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
		}
		contents.Clear();

	}
	else
	{
		int type = PMOVE_USER;
		std::string bytestring;
		MOVE_USER::CONTENTS contents;

		if ((*HandleVector)[i].handleInfo->char_id != -13)
		{
			auto element = contents.mutable_data()->Add();
			element->set_id((*HandleVector)[i].handleInfo->char_id);
			element->set_xoff(dxy[randInt % 4][0]);
			element->set_yoff(dxy[randInt % 4][1]);

			contents.SerializeToString(&bytestring);
			int len = bytestring.length();

			ioInfo = ioInfoPool->popBlock();
			ioInfo->block = MemoryPool->popBlock();

			copy_to_buffer(ioInfo->block->getBuffer(), &type, &len, &bytestring);
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
			ioInfo->wsaBuf.len = len + sizeof(int)* 2;
			ioInfo->RWmode = WRITE;

			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
			contents.Clear();
		}

	}
	HANDLE hTimer = NULL;
	auto pi = i;
	auto res = CreateTimerQueueTimer(&hTimer, TimerQueue, (WAITORTIMERCALLBACK)TimerRoutine, lpParam, (*HandleVector)[i].tic, 0, 0);
	if (res == 0)
	{
		printLog("Error (%d : %d)T.T\n", i, GetLastError());
		return;
	}
}

void sender(std::vector<handledata>* HandleVector)
{
	auto HandlerPool = Handler_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();

	vec = HandleVector;
	HandleVector = NULL;

	TimerQueue = CreateTimerQueue();

	int* pi = new int[vec->size()];
	for (int i = 0; i < vec->size(); i++)
	{
		HANDLE hTimer = NULL;
		pi[i] = i;
		auto res = CreateTimerQueueTimer(&hTimer, TimerQueue, (WAITORTIMERCALLBACK)TimerRoutine, &pi[i], (*vec)[i].tic, 0, 0);
		if (res == 0)
		{
			printLog("Error (%d : %d)T.T\n", i, GetLastError());
			continue;
		}

	}
	/*
	while (true)
	{
	LPPER_IO_DATA ioInfo;

	for (int i = 0; i < HandleVector->size(); ++i)
	{
	(*HandleVector)[i].tic -= 1;
	if ((*HandleVector)[i].tic == 0)
	{
	int randInt = moveRand;
	(*HandleVector)[i].tic = randInt;
	SOCKET sock = (*HandleVector)[i].handleInfo->hClntSock;

	if ((*HandleVector)[i].state == PCONNECT)
	{
	(*HandleVector)[i].state = PINIT;
	int type = PCONNECT;

	std::string bytestring;
	CONNECT::CONTENTS contents;
	std::string* buff_msg = contents.mutable_data();
	*buff_msg = "HELLO SERVER!";

	contents.SerializeToString(&bytestring);
	int len = bytestring.length();

	ioInfo = ioInfoPool->popBlock();
	ioInfo->block = MemoryPool->popBlock();

	copy_to_buffer(ioInfo->block->getBuffer(), &type, &len, &bytestring);
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
	ioInfo->wsaBuf.len = len + sizeof(int)* 2;
	ioInfo->RWmode = WRITE;

	int ret = WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

	if (ret == SOCKET_ERROR)
	{
	if (WSAGetLastError() == ERROR_IO_PENDING)
	{
	printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
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
	printLog("Send Error (%d)\n", WSAGetLastError());
	}
	else
	{
	printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
	}
	contents.Clear();

	}
	else
	{
	int type = PMOVE_USER;
	std::string bytestring;
	MOVE_USER::CONTENTS contents;

	if ((*HandleVector)[i].handleInfo->char_id == -13) continue;
	auto element = contents.mutable_data()->Add();
	element->set_id((*HandleVector)[i].handleInfo->char_id);
	element->set_xoff(dxy[randInt % 4][0]);
	element->set_yoff(dxy[randInt % 4][1]);

	contents.SerializeToString(&bytestring);
	int len = bytestring.length();

	ioInfo = ioInfoPool->popBlock();
	ioInfo->block = MemoryPool->popBlock();

	copy_to_buffer(ioInfo->block->getBuffer(), &type, &len, &bytestring);
	memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
	ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
	ioInfo->wsaBuf.len = len + sizeof(int)* 2;
	ioInfo->RWmode = WRITE;

	WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
	contents.Clear();

	}
	}
	}
	//		Sleep(1);
	}
	*/

	while (true) Sleep(1000);
}
