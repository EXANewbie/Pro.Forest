#ifndef TYPES_H
#define TYPES_H

#define PRINT_LOG

#define connectRand (rand()%1000)+1
#define moveRand (rand()%200)+101

//const int BUFFER_SIZE = 1024;
const int Port = 78911;
const int READ = 3, WRITE = 5;
const int NOT_JOINED = -13;
const int UNDEFINED = -71;

const int HEADER_SIZE = 2 * sizeof(int);

const int BLOCK_COUNT = 100000;
const int BLOCK_SIZE = (1 << 14);

const int HANDLER_SIZE = 100000;
//const char* CONMSG = "HELLO SERVER!";
//const char* DSCMSG = "BYE SERVER!";

enum TYPE {
	PCONNECT, PINIT, PSET_USER, PMOVE_USER, PDISCONN, PERASE_USER
};

enum CAST {
	NOT_CAST, SINGLE_CAST, MULTI_CAST, BROAD_CAST
};

const int dxy[4][2]{{ -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }};

#endif