#include <vector>

#include "cmap.h"
#include "types.h"

using namespace std;

void send_erase_user_message(SYNCHED_CHARACTER_MAP *chars, vector< pair<int, SOCKET> >& errors)
{
	// 전송 실패한 유저들을 모아서 전송
	int type = ERASE_USER;
	int len = errors.size()*sizeof(int);
	char buff[1024];
	char *pBuf = buff;

	if (len == 0)
		return;

	vector< pair<int, SOCKET> > repeat_errors;

	for (int i = 0; i < errors.size(); i++)
	{
		memcpy(pBuf, &errors[i].first, sizeof(int));
		pBuf += sizeof(int);
	}

	for (int i = 0; i < errors.size(); i++)
	{
		printf("error occured (char id : %d, socket : %d) \n", errors[i].first, errors[i].second);
		chars->erase(errors[i].second);
		closesocket(errors[i].second);
	}
	printf("erase success!\n");

	chars->lock();
	for (auto itr = chars->begin(); itr != chars->end(); itr++)
	{
		auto ret = send(itr->first, (char *)&type, sizeof(int), 0);
		if (ret != sizeof(int))
		{
			Character c = *chars->find(itr->first);
			repeat_errors.push_back(make_pair(c.getID(), itr->first));
		}
		send(itr->first, (char *)&len, sizeof(int), 0);
		send(itr->first, (char *)buff, len, 0);
	}
	chars->unlock();

	if (!repeat_errors.empty())
	{
		send_erase_user_message(chars, repeat_errors);
	}
}