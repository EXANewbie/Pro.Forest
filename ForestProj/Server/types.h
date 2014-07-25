#ifndef TYPES_H
#define TYPES_H

//#define PRINT_LOG

#include "Constant.h"

enum TYPE {
	PCONNECT, PINIT, PSET_USER, PMOVE_USER, PDISCONN, PERASE_USER
};

enum CAST {
	NOT_CAST, SINGLE_CAST, MULTI_CAST, BROAD_CAST
};

#endif