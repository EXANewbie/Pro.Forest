#include <string>
#include <cstdio>     
#include <cstdarg>

#include "types.h"

void printLog(const char *msg, ...)
{
#ifdef PRINT_LOG
	const int BUF_SIZE = 512;
	char buf[BUF_SIZE] = { 0, };
	va_list ap;

	strcpy_s(buf, "Log : ");
	va_start(ap, msg);
	vsprintf_s(buf + strlen(buf), BUF_SIZE - strlen(buf), msg, ap);
	va_end(ap);

	puts(buf);
#endif;
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