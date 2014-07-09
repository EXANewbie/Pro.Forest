#ifndef SYNCHED_LIST_H
#define SYNCHED_LIST_H

#include <WinSock2.h>
#include <list>
#include <mutex>

using std::list;
using std::mutex;

class Synched_List
{
private :
	list<SOCKET> slist;
	static mutex mtx;
	static Synched_List *instance;
	Synched_List() {}
public :
	static Synched_List *getInstance()
	{
		if (instance != NULL)
			return instance;

		mtx.lock();
		if (instance == NULL)
		{
			instance = new Synched_List();
		}
		mtx.unlock();

		return instance;
	}

	list<SOCKET>::iterator begin()
	{
		return slist.begin();
	}

	list<SOCKET>::iterator end()
	{
		return slist.end();
	}

	void erase(list<SOCKET>::iterator& itr)
	{
		mtx.lock();
		slist.erase(itr);
		mtx.unlock();
	}

	void push_back(SOCKET s)
	{
		mtx.lock();
		slist.push_back(s);
		mtx.unlock();
	}
};

#endif