#ifndef USER_PAIR_H
#define USER_PAIR_H

#include <WinSock2.h>
#include "character.h"

struct userSC {
	SOCKET socket;
	Character character;
	userSC()
	{
	}
	userSC(SOCKET socket, Character character)
	{
		this->socket = socket;
		this->character = character;
	}
	void operator =(const userSC A) 
	{
		socket = A.socket;
		character = A.character;
	}

	~userSC()
	{
//		if ( character != NULL )
//			delete character ;
	}
};
#endif