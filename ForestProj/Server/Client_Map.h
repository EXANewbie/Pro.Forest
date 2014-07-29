#ifndef CLIENT_MAP_H
#define CLIENT_MAP_H

#include <map>
#include "character.h"
#include <WinSock2.h>
#include <mutex>

using std::map;
using std::pair;
//using std::mutex;
class Client_Map
{
private :
	map<int, SOCKET> id_sock;
	map<SOCKET, int> sock_id;
	map<int, Character*> id_char;
	
	//static mutex mtx;
	SRWLOCK srw;
	static Client_Map *instance;
	Client_Map()
	{
		InitializeSRWLock(&srw);
	}
public :
	static Client_Map *getInstance()
	{
		if (instance == NULL)
		{
			instance = new Client_Map();
		}
		return instance;
	}
	SOCKET find_id_to_sock(int);
	Character *find_id_to_char(int);
	Character *find_sock_to_char(SOCKET);
	int find_sock_to_id(SOCKET);

	void erase(int);
	void erase(SOCKET);
	bool insert(int, SOCKET, Character*);
	
	map<int, Character*>::iterator begin();
	map<int, Character*>::iterator end();

	void Rlock();
	void Runlock();
	void Wlock();
	void Wunlock();
};

#endif