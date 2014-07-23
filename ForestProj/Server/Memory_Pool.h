#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stack>
#include <mutex>
#include <cassert>
#include <memory>

#include "types.h"
#include "Completion_Port.h"

using std::mutex;
using std::stack;
using std::shared_ptr;
using std::make_shared;

class Memory_Block {
private :
	bool is_used;
	int size, MAXSIZE;
	char *memory;
	friend class Memory_Pool;
private :
	Memory_Block(int size) {
		is_used = false;
		this->size = 0;
		MAXSIZE = size;
		memory = new char[size];
	}
private :
	void setStateUSE() {
		is_used = true;
		size = 0;
	}
	void setStateNOTUSE() {
		is_used = false;
	}
public :
	Memory_Block() : Memory_Block(BLOCK_SIZE) {}
	~Memory_Block() {
		delete[] memory;
	}
	
	int getSize() { return size; }
	char *getBuffer() { return memory; }

	void copy(int offset, char *buffer, int len) {
		assert(is_used); // 만약 지금 사용할 수 없는데 접근한 경우, Access Denied!

		assert(offset >= 0);
		assert(offset + len <= MAXSIZE); // 만약 지금 복사하려는 영역이 할당 영역 밖이라면, Access Denied!

		assert(len >= 0); // 음수값 대입 방지

		memcpy(memory + offset, buffer, len);

		if (size < offset + len) {
			size = offset + len;
		}
	}
};
/*
struct MB_deleter {
	void operator()(Memory_Block *m) {
		delete m;
	}
};
*/
class Memory_Pool {
private :
	typedef Memory_Block* ptr;//shared_ptr<Memory_Block> ptr;
	stack<ptr> *poolStack;
	mutex mtx;
private :
	Memory_Pool(int size) {
		poolStack = new stack<ptr>;
		for (int i = 0; i < size; i++) {
			poolStack->push(new Memory_Block());
		}
	}
public :
	Memory_Pool() : Memory_Pool(BLOCK_COUNT) {}
	~Memory_Pool() {
		delete poolStack;
	}
	ptr popBlock() {
		mtx.lock();
		ptr p = poolStack->top();
		poolStack->pop();
		mtx.unlock();

		p->setStateUSE();

		return p;
	}
	void pushBlock(ptr p) {
		p->setStateNOTUSE();

		mtx.lock();
		poolStack->push(p);
		mtx.unlock();
	}
};

class Handler_Pool {
private:
	typedef LPPER_HANDLE_DATA ptr;
	stack<ptr> *poolStack;

	static mutex mtx;
	static Handler_Pool *instance;
private:
	Handler_Pool(int size) {
		poolStack = new stack<ptr>;
		for (int i = 0; i < size; i++) {
			poolStack->push(ptr());
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

		mtx.lock();
		if (instance == NULL) {
			instance = new Handler_Pool;
		}
		mtx.unlock();

		return instance;
	}
	ptr popBlock() {
		mtx.lock();
		ptr p = poolStack->top();
		poolStack->pop();
		mtx.unlock();

		return p;
	}
	void pushBlock(ptr p) {
		mtx.lock();
		poolStack->push(p);
		mtx.unlock();
	}
};

class ioInfo_Pool {
private:
	typedef LPPER_IO_DATA ptr;
	stack<ptr> *poolStack;

	static mutex mtx;
	static ioInfo_Pool *instance;
private:
	ioInfo_Pool(int size) {
		poolStack = new stack<ptr>;
		for (int i = 0; i < size; i++) {
			poolStack->push(ptr());
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

		mtx.lock();
		if (instance == NULL) {
			instance = new ioInfo_Pool;
		}
		mtx.unlock();

		return instance;
	}
	ptr popBlock() {
		mtx.lock();
		ptr p = poolStack->top();
		poolStack->pop();
		mtx.unlock();

		return p;
	}
	void pushBlock(ptr p) {
		mtx.lock();
		poolStack->push(p);
		mtx.unlock();
	}
};
#endif