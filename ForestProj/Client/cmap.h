#ifndef CMAP_H
#define CMAP_H
#include <map>
#include <mutex>
#include <WinSock2.h>

#include "character.h"

struct SYNCHED_CHARACTER_MAP
{
	typedef std::map<int, Character> MAP;
	MAP CM;
	std::mutex mtx;

	void lock()
	{
		mtx.lock();
	}

	void unlock()
	{
		mtx.unlock();
	}

	MAP::iterator begin()
	{
		return CM.begin();
	}

	MAP::iterator end()
	{
		return CM.end();
	}

	void erase(int key)
	{
		CM.erase(key);
	}

	void insert(int key, Character& value)
	{
		CM[key] = value;
	}

	Character* find(int key) {
		return &CM[key];
	}
};


#endif
