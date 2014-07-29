#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stack>
#include <mutex>
#include <cassert>
#include <memory>

#include "types.h"
#include "Completion_Port.h"
#include "Memory_Block.h"

using std::mutex;
using std::stack;
using std::shared_ptr;

void printLog(const char *msg, ...);
/*
struct MB_deleter {
	void operator()(Memory_Block *m) {
		delete m;
	}
};
*/

class Memory_Pool {
private :
	typedef Memory_Block data;
	typedef data* ptr;//shared_ptr<Memory_Block> ptr;
	stack<ptr> *poolStack;

	//static mutex mtx;
	SRWLOCK srw;
	static Memory_Pool *instance;
private :
	Memory_Pool(int size) {
		poolStack = new stack<ptr>;
		InitializeSRWLock(&srw);
		for (int i = 0; i < size; i++)
		{
			poolStack->push(new data());
		}
	}
	Memory_Pool() : Memory_Pool(BLOCK_COUNT) {}
	~Memory_Pool() {
		delete poolStack;
	}
public :
	static Memory_Pool *getInstance() {
		if (instance != NULL)
			return instance;

		if (instance == NULL) {
			instance = new Memory_Pool;
		}

		return instance;
	}
	ptr popBlock() {
		AcquireSRWLockExclusive(&srw);
		ptr p = poolStack->top();
		poolStack->pop();
		printLog("MemoryBlock Allocated(%d)\n", poolStack->size());
		ReleaseSRWLockExclusive(&srw);
		p->setStateUSE();

		return p;
	}
	void pushBlock(ptr p) {
		p->setStateNOTUSE();

		AcquireSRWLockExclusive(&srw);
		poolStack->push(p);
		printLog("MemoryBlock released(%d)\n", poolStack->size());
		ReleaseSRWLockExclusive(&srw);
	}
};

class Handler_Pool {
private:
	typedef PER_HANDLE_DATA data;
	typedef data* ptr;
	stack<ptr> *poolStack;

	SRWLOCK srw;
	static Handler_Pool *instance;
private:
	Handler_Pool(int size) {
		poolStack = new stack<ptr>;
		InitializeSRWLock(&srw);
		for (int i = 0; i < size; i++) 
		{
			poolStack->push(new data());
		}
	}
	Handler_Pool() : Handler_Pool(HANDLER_SIZE) {}
	~Handler_Pool() {
		delete poolStack;
	}
public:
	static Handler_Pool *getInstance() {
		if (instance != NULL)
			return instance;

		if (instance == NULL) {
			instance = new Handler_Pool;
		}

		return instance;
	}
	ptr popBlock() {
		AcquireSRWLockExclusive(&srw);
		ptr p = poolStack->top();
		poolStack->pop();
		printLog("HandlerBlock Allocated(%d)\n", poolStack->size());
		ReleaseSRWLockExclusive(&srw);

		return p;
	}
	void pushBlock(ptr p) {
		AcquireSRWLockExclusive(&srw);
		poolStack->push(p);
		printLog("HandlerBlock released(%d)\n", poolStack->size());
		ReleaseSRWLockExclusive(&srw);
	}
};

class ioInfo_Pool {
private:
	typedef PER_IO_DATA data;
	typedef data* ptr;
	stack<ptr> *poolStack;

	SRWLOCK srw;
	static ioInfo_Pool *instance;
private:
	ioInfo_Pool(int size) {
		poolStack = new stack<ptr>;
		InitializeSRWLock(&srw);
		for (int i = 0; i < size; i++) {
			poolStack->push(new data());
		}
	}
	ioInfo_Pool() : ioInfo_Pool(HANDLER_SIZE) {}
	~ioInfo_Pool() {
		delete poolStack;
	}
public:
	static ioInfo_Pool *getInstance() {
		if (instance != NULL)
			return instance;

		if (instance == NULL) {
			instance = new ioInfo_Pool;
		}

		return instance;
	}
	ptr popBlock() {
		AcquireSRWLockExclusive(&srw);
		ptr p = poolStack->top();
		poolStack->pop();
		printLog("ioInfoBlock Allocated(%d)\n", poolStack->size());
		ReleaseSRWLockExclusive(&srw);

		return p;
	}
	void pushBlock(ptr p) {
		AcquireSRWLockExclusive(&srw);
		poolStack->push(p);
		printLog("ioInfoBlock released(%d)\n", poolStack->size());
		ReleaseSRWLockExclusive(&srw);
	}
};
#endif