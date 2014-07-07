#include <WinSock2.h>

#include <iostream>
#include <mutex>
#include <map>

#include "que.h"
#include "msg.h"
#include "types.h"
using namespace std;

#define END_MSG "\\QUIT"

void each_client(SOCKET Connection, int User_ID, SYNCHED_QUEUE *que) {
	int type;
	int len;
	char Buff[1024];

	printf("User %d is connected\n", User_ID);

	// GET IP ADDRESS
	SOCKADDR_IN temp_sock;
	int temp_sock_size = sizeof(temp_sock);
	getpeername(Connection, (SOCKADDR *)&temp_sock, &temp_sock_size);
	cout << "Connect IP : " << inet_ntoa(temp_sock.sin_addr) << endl;

	do
	{
		auto ret = recv(Connection, (char *)&type, sizeof(int), 0);
		if (ret != sizeof(int)) // 비정상적인 종료의 경우
		{
			printf("User %d is disconnected\n", User_ID);
			break;
		}

		recv(Connection, (char *)&len, sizeof(int), 0);
		
		int USER_ID_SIZE = sizeof(int) / sizeof(char);

		char* pBuf = Buff;
		memcpy(pBuf, &User_ID, sizeof(int));
		pBuf += sizeof(int);

		int inc = 0;
		do
		{
			auto ret = recv(Connection, pBuf, len, 0);

			if (ret == EOF || ret == SOCKET_ERROR)
			{
				// 클라이언트가 죽었습니다. ㅠㅠ
				break;
			}
			pBuf += ret;
			inc += ret;
		} while (inc < len);

		if (inc > len)
		{
			// 패킷이... 이상한데?
		}
		//auto ret = recv(Connection, pBuf, len, 0);
		
		pBuf += ret;
		*pBuf = '\0';
		len += USER_ID_SIZE;

		int temp_user;
		
		pBuf = Buff;
		memcpy(&temp_user, Buff, sizeof(int));

		printf("User %d : %s\n", temp_user, pBuf);

		que->push(msg(type,len,Buff));
	} while (true);

	closesocket(Connection); // 연결 소켓을 닫는다.
}