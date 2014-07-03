#ifndef MSG_H
#define MSG_H

#include <cstring>

struct msg {
	int type;
	int len;
	char buff[1024];

	msg(int type, int len, char *buff) {
		this->type = type;
		this->len = len;
		strncpy_s(this->buff, buff,strlen(buff));
	}
};

#endif