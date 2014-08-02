#ifndef TYPES_H
#define TYPES_H

//#define PRINT_LOG

#include "Constant.h"

enum TYPE {
	PCONNECT, PINIT, PSET_USER, PMOVE_USER, PDISCONN, PERASE_USER, PSET_MON, PERASE_MON, PUSER_ATTCK, PUSER_ATTCK_RESULT, PMODEPEACEMOVE//ai관련 패킷은 뒤로 따로
};

enum CAST {
	NOT_CAST, SINGLE_CAST, MULTI_CAST, BROAD_CAST
};

enum AITYPE {
	PHELLOWORLD
};

enum INFOTYPE {
	READ = 3, WRITE, TIMER
};

enum STATE_MON {
	PEACE, BATTLE, DEAD
};
#endif