#ifndef CLIENT_MAP_H
#define CLIENT_MAP_H

#include <map>
#include "character.h"
#include <WinSock2.h>

using std::map;
using std::pair;

class Client_Map
{
private :
	map<int, SOCKET> id_sock;
	map<SOCKET, int> sock_id;
	map<int, Character> id_char;
public :
	SOCKET find_id_to_sock(int);
	Character *find_id_to_char(int);
	Character *find_sock_to_char(SOCKET);
	int find_sock_to_id(SOCKET);

	void erase(int);
	void insert(int, SOCKET);
	
	map<int, Character>::iterator begin();
	map<int, Character>::iterator end();
};

#endif