#include "Client_Map.h"

//class Client_Map
//{
//private:
//	map<int, SOCKET> id_sock;
//	map<SOCKET, int> sock_id;
//	map<int, Character> id_char;
//public:
//	SOCKET find_id_to_sock(int);
//	Character *find_id_to_char(int);
//	Character *find_sock_to_char(SOCKET);
//	int find_sock_to_id(SOCKET);
//
//	void erase(int);
//	void insert(int, SOCKET);
//
//	pair<int, Character> begin();
//	pair<int, Character> end();
//};

SOCKET Client_Map::find_id_to_sock(int id)
{
	auto ret = id_sock.find(id);
	if (ret == id_sock.end())
	{
		return SOCKET_ERROR;
	}
	else
	{
		return ret->second;
	}
}
Character* Client_Map::find_id_to_char(int id)
{
	auto ret = id_char.find(id);
	if (ret == id_char.end())
	{
		return NULL;
	}
	else
	{
		return ret->second;
	}
}
Character* Client_Map::find_sock_to_char(SOCKET sock)
{
	auto ret = find_sock_to_id(sock);
	if (ret == -1)
	{
		return NULL;
	}
	else
	{
		return find_id_to_char(ret);
	}
}
int Client_Map::find_sock_to_id(SOCKET sock)
{
	auto ret = sock_id.find(sock);

	if (ret == sock_id.end())
	{
		return -1;
	}
	else
	{
		return ret->second;
	}
}

void Client_Map::erase(int id)
{
	auto sock = find_id_to_sock(id);
	
	if (sock != SOCKET_ERROR)
	{
		sock_id.erase(sock);
		id_sock.erase(id);
	}

	auto cha = find_id_to_char(id);

	if (cha != NULL)
	{
		delete cha;
		id_char.erase(id);
	}
}

void Client_Map::erase(SOCKET sock)
{
	auto id = find_sock_to_id(sock);

	if (sock != SOCKET_ERROR)
	{
		sock_id.erase(sock);
		id_sock.erase(id);
	}

	auto cha = find_id_to_char(id);

	if (cha != NULL)
	{
		id_char.erase(id);
	}
}

bool Client_Map::insert(int id, SOCKET sock, Character* now)
{
	if (sock_id.find(sock) != sock_id.end())
	{
		return false;
	}
	sock_id.insert(pair<SOCKET, int>(sock, id));
	id_sock.insert(pair<int, SOCKET>(id, sock));
	id_char.insert(pair<int, Character*>(id, now));
	//id_char[id] = now;
	return true;
}

map<int, Character*>::iterator Client_Map::begin()
{
	return id_char.begin();
}

map<int, Character*>::iterator Client_Map::end()
{
	return id_char.end();
}

void Client_Map::Rlock() {
	//mtx.lock();
	AcquireSRWLockShared(&srw);
}

void Client_Map::Runlock() {
	//mtx.unlock();
	ReleaseSRWLockShared(&srw);
}
void Client_Map::Wlock() {
	//mtx.lock();
	AcquireSRWLockExclusive(&srw);
}

void Client_Map::Wunlock() {
	//mtx.unlock();
	ReleaseSRWLockExclusive(&srw);
}