#ifndef COMPLETION_PORT_H
#define COMPLETION_PORT

#include <WinSock2.h>
#include "types.h"
#include "character.h"

typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDEL_DATA, *LPPER_HANDLE_DATA;

typedef struct
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUFFER_SIZE];
	int RWmode;
	int char_id;
	Character* my_char;
}PER_IO_DATA, *LPPER_IO_DATA;

#endif