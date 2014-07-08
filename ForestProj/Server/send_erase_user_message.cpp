#include <vector>

#include "types.h"
//#include "cmap.h"

using namespace std;

void send_erase_user_message(Client_Map *CMap, vector< pair<int, SOCKET> >& errors)
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
		CMap->erase(errors[i].first);
		closesocket(errors[i].first);
	}
	printf("erase success!\n");

	
	for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
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


	if (!repeat_errors.empty())
	{
		send_erase_user_message(CMap, repeat_errors);
	}
}