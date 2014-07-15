#ifndef SOCK_SET_H
#define SOCK_SET_H

#include <WinSock2.h>
#include <set>
#include <mutex>

class Sock_set
{
private :
	std::set<SOCKET> s_set;
	static std::mutex mtx;

	static Sock_set *instance;
	Sock_set() {}
public :
	static Sock_set *getInstance()
	{
		if (instance != NULL)
			return instance;

		mtx.lock();
		if (instance == NULL)
			instance = new Sock_set();
		mtx.unlock();

		return instance;
	}
	void insert(SOCKET sock)
	{
		mtx.lock();
		s_set.insert(sock);
		mtx.unlock();
	}
	void erase(SOCKET sock)
	{
		mtx.lock();
		s_set.erase(sock);
		mtx.unlock();
	}
};

#endif