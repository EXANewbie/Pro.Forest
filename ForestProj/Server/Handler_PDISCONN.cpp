#include <string>

#include "../protobuf/disconn.pb.h"

#include "Completion_Port.h"

/*
#include "Check_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "character.h"
#include "Sock_set.h"
#include "DMap.h"
#include "Scoped_Lock.h"

#include "Memory_Pool.h"
#include "monster.h"
#include "DMap_monster.h"
#include "msg.h"
*/

using std::string;

void printLog(const char *msg, ...);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);

void Handler_PDISCONN(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo, string* readContents)
{
	DISCONN::CONTENTS disconn;

	disconn.ParseFromString(*readContents);
	if (disconn.data() != "BYE SERVER!")
	{
		//가짜 클라이언트
	}

	printLog("Nomal turn off\n");
	remove_valid_client(handleInfo, ioInfo);
}