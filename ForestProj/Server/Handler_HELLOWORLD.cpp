#include <string>

#include "Completion_Port.h"
#include "Memory_Pool.h"

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

void Handler_HELLOWORLD(LPPER_IO_DATA ioInfo, string* readContents) {
	auto MemoryPool = Memory_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	if (ioInfo->block != nullptr) {
		MemoryPool->pushBlock(ioInfo->block);
	}
	ioInfoPool->pushBlock(ioInfo);
	printLog("Hello\n");

	/*	auto timer = Timer::getInstance();*/

	char *str = "Hello World!";
	int type = PHELLOWORLD, len = strlen(str);

	char arr[25];

	memcpy(arr, &type, sizeof(int));
	memcpy(arr + sizeof(int), &len, sizeof(int));
	memcpy(arr + 2 * sizeof(int), str, len);
	/*	timer->addSchedule(1000, string(arr,len+2*sizeof(int)));*/
}
