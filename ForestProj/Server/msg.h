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
		for (int i = 0; i < len; i++)
			this->buff[i] = buff[i];
		this->buff[len] = '\0';
	}
};

#endif