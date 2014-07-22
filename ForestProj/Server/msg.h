#ifndef MSG_H
#define MSG_H

#include <cstring>

struct msg {
	int type;
	int len;
	const char *buff;

	msg(int type, int len, const char *buff) {
		this->type = type;
		this->len = len;
		this->buff = buff;
	}
};

#endif