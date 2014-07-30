#include <memory>
#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>
#include <string>

#include "character.h"
#include "cmap.h"
#include "types.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"

void send_move(const SOCKET s, const char& c, const int& myID);
void copy_to_buffer(char* pBuf, int* type, int* len, std::string* content);
int bigRand();

void send_move(const SOCKET s, const char& c, const int& myID)
{
	int x_off, y_off;
	char buf[1024];
	int len;
	TYPE type = PMOVE_USER;

	if (c == 'w') { x_off = 0; y_off = -1; }
	else if (c == 'a'){ x_off = -1;	y_off = 0; }
	else if (c == 's'){ x_off = 0;	y_off = 1; }
	else if (c == 'd'){ x_off = 1;	y_off = 0; }

	MOVE_USER::CONTENTS contents;
	auto element = contents.mutable_data()->Add();
	element->set_id(myID);
	element->set_xoff(x_off);
	element->set_yoff(y_off);

	std::string bytestring;
	contents.SerializeToString(&bytestring);
	len = bytestring.length();

	copy_to_buffer(buf, (int *)&type, &len, &bytestring);

	send(s, buf, len + sizeof(int)* 2, 0);
}

void copy_to_buffer(char* pBuf, int* type, int* len, std::string* content)
{
	memcpy(pBuf, (char*)type, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, (char *)len, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, content->c_str(), *len);
}

int bigRand()
{
	return (rand() << 15) + rand();
}