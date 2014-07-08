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

	void insert(SOCKET s,int id)
	{
		mtx.lock();
		DMap.insert(pair<SOCKET, int>(s, id));
		mtx.unlock();
	}
	void erase(SOCKET s)
	{
		mtx.lock();
		DMap.erase(s);
		mtx.unlock();
	}
	int find(SOCKET s)
	{
		mtx.lock();
		int ret = DMap[s];
		mtx.unlock();
		return ret;
	}
};

#endif