#ifndef TYPES_H
#define TYPES_H

const int BUFFER_SIZE = 1024;
const int Port = 78911;
const int READ = 3, WRITE = 5;
//const char* CONMSG = "HELLO SERVER!";
//const char* DSCMSG = "BYE SERVER!";

enum TYPE {
	PCONNECT, PINIT, PSET_USER, PMOVE_USER, PDISCONN, PERASE_USER
};

enum CAST {
	NOT_CAST, SINGLE_CAST, MULTI_CAST, BROAD_CAST
};

#endif