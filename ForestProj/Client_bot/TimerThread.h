#ifndef TIMER_H
#define TIMER_H

#include <string>
#include <thread>
#include <map>
#include <vector>
#include <utility>

#include <process.h>
#include <WinSock2.h>
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
void TimerThreadProc(Timer*);

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
		t1 = new thread(TimerThreadProc, instance);
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
		key /= TICK; // get Tick Count
		key += nowTime; // nowTime + Tick Count
		MAPS.insert(make_pair(key, value));
		unlock();
	}
	vector<string> getSchedule() {
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
#endif