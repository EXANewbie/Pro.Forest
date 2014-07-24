#ifndef COMPLETION_PORT_H
#define COMPLETION_PORT_H

#include <WinSock2.h>
#include "character.h"

#include "Memory_Block.h"

typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
	int char_id;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[8];
	Memory_Block *block;
	int RWmode;
	int type, len, offset;
	int id;
	Character* myCharacter;
}PER_IO_DATA, *LPPER_IO_DATA;

#endif