#ifndef TIMER_H
#define TIMER_H

#include <string>
#include <thread>
#include <map>
#include <vector>
#include <utility>

#include <process.h>
#include <Windows.h>

#include "Completion_Port.h"
#include "types.h"
#include "Memory_Block.h"
#include "Memory_Pool.h"

using std::multimap;
using std::thread;
using std::string;
using std::vector;

class Timer;
void ThreadProc(Timer*);

class Timer {
private:
	HANDLE ComPort;
	const int SECOND = 1000;
	int TICK_PER_SECOND, TICK;
	int nowTime;

	multimap<int, string> MAPS;
	thread* t1;

	CRITICAL_SECTION cs;

	bool isStarted;

	static Timer *instance;
private:
	Timer(int fps) {
		InitializeCriticalSection(&cs);
		TICK_PER_SECOND = fps;
		TICK = SECOND / TICK_PER_SECOND;
		isStarted = false;

		nowTime = 0;
	}
	Timer(HANDLE CompletionPort) : Timer(60){
		ComPort = CompletionPort;
	}
	Timer() : Timer(60) {}
	void GetStartThread() {
		isStarted = true;
		t1 = new thread(ThreadProc, instance);
	}
	void lock() {
		EnterCriticalSection(&cs);
	}
	void unlock() {
		LeaveCriticalSection(&cs);
	}
public:
	void start() {
		GetStartThread();
	}
	void addSchedule(int key, string& value) {
		lock();
		key /= TICK_PER_SECOND; // get Tick Count
		key += nowTime; // nowTime + Tick Count
		MAPS.insert(make_pair(key, value));
		unlock();
	}
	vector<string>& getSchedule() {
		vector<string> ret;
		lock();
		while (MAPS.begin() != MAPS.end()) {
			auto itr = MAPS.begin();
			if (itr->first > nowTime) {
				break;
			}
			ret.push_back(itr->second);
			MAPS.erase(itr);
		}
		nowTime++;
		unlock();

		return ret;
	}
	inline HANDLE getCompletionPort() {
		return ComPort;
	}
	inline int getTick() {
		return TICK;
	}
	inline void setCompletionPort(HANDLE CompletionPort) {
		ComPort = CompletionPort;
	}
public:
	static Timer *getInstance() {
		if (instance != nullptr)
			return instance;

		instance = new Timer();

		return instance;
	}
};
void ThreadProc(Timer* T) {
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
			string contents(i->c_str(), len);

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = len;	/* 이 부분에서 BUFFER_SIZE를 필히 수정해야됨, (엄밀히 말하면 8바이트만 먼저 받도록)*/
			ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
			ioInfo->RWmode = READ;
			ioInfo->id = NOT_JOINED;
			ioInfo->myCharacter = NULL;
			ioInfo->type = type;
			ioInfo->len = len;
			ioInfo->offset = 0;
			memcpy(ioInfo->block->getBuffer(), contents.c_str(), len);

			ioInfo->myCharacter = nullptr;

			PostQueuedCompletionStatus(T->getCompletionPort(), (DWORD)i->size(), NULL, (LPOVERLAPPED)&ioInfo);
			//				GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);
		}

		SleepEx(T->getTick(), false);
	}
}

#endif