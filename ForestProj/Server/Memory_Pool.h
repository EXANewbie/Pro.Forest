#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stack>
#include <mutex>
#include <cassert>
#include <memory>

#include "types.h"

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

struct MB_deleter {
	void operator()(Memory_Block *m) {
		delete m;
	}
};

class Memory_Pool {
private :
	typedef shared_ptr<Memory_Block> ptr;
	stack<ptr> *poolStack;
	mutex mtx;
private :
	Memory_Pool(int size) {
		poolStack = new stack<ptr>;
		for (int i = 0; i < size; i++) {
			poolStack->push(ptr(new Memory_Block(), MB_deleter()));
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

#endif