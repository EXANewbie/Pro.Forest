#ifndef DISC_USER_MAP_H
#define DISC_USER_MAP_H

#include <map>
#include <WinSock2.h>
#include <mutex>

using std::map;
using std::pair;
using std::mutex;

class Disc_User_Map
{
private:
	map<SOCKET, int> DMap;
	static mutex mtx;
	static Disc_User_Map *instance;
	Disc_User_Map(){}
public:

	static Disc_User_Map *getInstance()
	{
		if (instance != NULL)
			return instance;

		mtx.lock();
		if (instance == NULL)
		{
			instance = new Disc_User_Map();
		}
		mtx.unlock();
		return instance;
	}

	void insert(pair<SOCKET ,int>& pairs)
	{
		mtx.lock();
		DMap.insert(pairs);
		mtx.unlock();
	}
	void erase(SOCKET s)
	{
		mtx.lock();
		DMap.erase(s);
		mtx.unlock();
	}
	void erase(map<SOCKET, int>::iterator s)
	{
		mtx.lock();
		DMap.erase(s);
		mtx.unlock();
	}
	map<SOCKET, int>::iterator begin()
	{
		mtx.lock();
		auto ret = DMap.begin();
		mtx.unlock();
		return ret;
	}
	map<SOCKET, int>::iterator end()
	{
		mtx.lock();
		auto ret = DMap.end();
		mtx.unlock();
		return ret;
	}
	map<SOCKET, int>::iterator find(SOCKET s)
	{
		mtx.lock();
		auto ret = DMap.find(s);
		mtx.unlock();
		return ret;
	}
};

#endif