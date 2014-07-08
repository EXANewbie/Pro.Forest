#ifndef QUE_H
#define QUE_H

#include <queue>
#include <mutex>

#include "msg.h"

class SYNCHED_QUEUE {
private :
	std::queue<msg> que;

	static std::mutex mtx;
	static SYNCHED_QUEUE *instance;
	SYNCHED_QUEUE() {}
public :
	static SYNCHED_QUEUE *getInstance() {
		if (instance != NULL)
			return instance;

		mtx.lock();
		if (instance == NULL)
		{
			instance = new SYNCHED_QUEUE();
		}
		mtx.unlock();

		return instance;
	}

	void push(msg);
	msg& front();
	void pop();
	int size();
	bool empty();
};

#endif