#include "TimerThread.h"

#include "Memory_Block.h"
#include "Memory_Pool.h"

void TimerThreadProc(Timer* T) {
	auto MemoryPool = Memory_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	while (true){
		auto vec = T->getSchedule();
		for (auto i = vec.begin(); i != vec.end(); i++) {
			LPPER_IO_DATA ioInfo = ioInfoPool->popBlock();
			ioInfo->block = MemoryPool->popBlock();

			int type, len;

			memcpy(&type, i->c_str(), sizeof(int));
			memcpy(&len, i->c_str() + sizeof(int), sizeof(int));
			string contents(i->c_str()+2*sizeof(int), len);

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = len;	/* �� �κп��� BUFFER_SIZE�� ���� �����ؾߵ�, (������ ���ϸ� 8����Ʈ�� ���� �޵���)*/
			ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
			ioInfo->RWmode = TIMER;
			ioInfo->id = NOT_JOINED;
			ioInfo->myCharacter = NULL;
			ioInfo->type = type;
			ioInfo->len = len;
			ioInfo->offset = 0;
			memcpy(ioInfo->block->getBuffer(), contents.c_str(), len);

			ioInfo->myCharacter = nullptr;

			PostQueuedCompletionStatus(T->getCompletionPort(), (DWORD)i->size(), NULL, (LPOVERLAPPED)ioInfo);
			//GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);
		}

		SleepEx(T->getTick(), false);
	}
}