#ifndef CHECK_MAP_H
#define CHECK_MAP_H

#include <WinSock2.h>
#include <map>

class Check_Map
{
private:
	std::map<SOCKET, int> sock_id;
	SRWLOCK srw;
	static Check_Map* instance;
	Check_Map()
	{
		InitializeSRWLock(&srw);
	}
public:
	static Check_Map* getInstance()
	{
		if (instance == NULL)
		{
			instance = new Check_Map;
		}
		return instance;
	}

	void insert(SOCKET sock, int id)
	{
		AcquireSRWLockExclusive(&srw);
		sock_id.insert(std:: pair<SOCKET, int>(sock, id));
		ReleaseSRWLockExclusive(&srw);
	}

	void erase(SOCKET sock, int id)
	{
		AcquireSRWLockExclusive(&srw);
		sock_id.erase(sock);
		ReleaseSRWLockExclusive(&srw);
	}

	bool check(SOCKET sock, int id)
	{
		AcquireSRWLockShared(&srw);
		auto find_loc = sock_id.find(sock);
		auto end_loc = sock_id.end();
		ReleaseSRWLockShared(&srw);

		//지워진 소켓
		if (find_loc == end_loc)
		{
			return false;
		}
		if (find_loc->second == id) return true;
		return false;

	}
	
};

#endif