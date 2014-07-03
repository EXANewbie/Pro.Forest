#ifndef QUE_H
#define QUE_H

#include <queue>
#include <mutex>

#include "msg.h"
struct SYNCHED_QUEUE {
	std::queue<msg> que;
	std::mutex mtx;
	
	void push(msg i) {
		lock();
		que.push(i);
		unlock();
	}

	msg front() {
		lock();
		msg t = que.front();
		unlock();
		return t;
	}

	void pop() {
		lock();
		que.pop();
		unlock();
	}
	
	int size() {
		lock();
		int t = que.size();
		unlock();
		return t;
	}

	bool empty() {
		lock();
		bool t = que.empty();
		unlock();
		return t;
	}

	void lock() {
		mtx.lock();
	}

	void unlock() {
		mtx.unlock();
	}
};

#endif