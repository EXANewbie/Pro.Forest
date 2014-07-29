#ifndef SOCK_SET_H
#define SOCK_SET_H

#include <WinSock2.h>
#include <set>
#include <mutex>

class Sock_set
{
private :
	std::set<SOCKET> s_set;
	SRWLOCK srw;
	static Sock_set *instance;
	Sock_set()
	{
		InitializeSRWLock(&srw);
	}
public :
	static Sock_set *getInstance()
	{
		if (instance != NULL)
			return instance;

		if (instance == NULL)
		{
			instance = new Sock_set();
		}
		return instance;
	}
	void insert(SOCKET sock)
	{
		AcquireSRWLockExclusive(&srw);
		s_set.insert(sock);
		ReleaseSRWLockExclusive(&srw);
	}
	void erase(SOCKET sock)
	{
		AcquireSRWLockExclusive(&srw);
		s_set.erase(sock);
		ReleaseSRWLockExclusive(&srw);
	}
};

#endif