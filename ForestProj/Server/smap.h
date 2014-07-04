#ifndef SMAP_H
#define SMAP_H
#include <map>
#include <mutex>
#include <WinSock2.h>

struct SYNCHED_SOCKET_MAP
{
	typedef std::map<int, SOCKET> MAP;
	MAP SM;
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
		return SM.begin();
	}

	MAP::iterator end()
	{
		return SM.end();
	}

	void erase(int key)
	{
		SM.erase(key);
	}

	void insert(int key, SOCKET value)
	{
		SM[key] = value;
	}

	SOCKET find(int key) {
		return SM[key];
	}
};


#endif
