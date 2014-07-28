#ifndef COMPLETION_PORT_H
#define COMPLETION_PORT_H

#include <WinSock2.h>

#include "Memory_Block.h"

int bigRand();

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
	Memory_Block *block;
	int RWmode;
}PER_IO_DATA, *LPPER_IO_DATA;

struct handledata
{
	LPPER_HANDLE_DATA handleInfo;
	int tic;
	int state;

	handledata(LPPER_HANDLE_DATA handleInfo)
	{
		this->handleInfo = handleInfo;
		tic = connectRand;
		state = PCONNECT;
	}
};

#endif