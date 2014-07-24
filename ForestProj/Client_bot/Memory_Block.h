#ifndef MEMORY_BLOCK_H
#define MEMORY_BLOCK_H

#include <cassert>
#include <cstring>
#include "types.h"

class Memory_Block {
private:
	bool is_used;
	int size, MAXSIZE;
	char *memory;
	friend class Memory_Pool;
private:
	Memory_Block(int size) {
		is_used = false;
		this->size = 0;
		MAXSIZE = size;
		memory = new char[size];
	}
private:
	void setStateUSE() {
		is_used = true;
		size = 0;
	}
	void setStateNOTUSE() {
		is_used = false;
	}
public:
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

#endif;