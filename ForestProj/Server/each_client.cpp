#include <WinSock2.h>

#include <iostream>
#include <mutex>
#include <map>

#include "que.h"
#include "msg.h"
#include "types.h"

using namespace std;

#define END_MSG "\\QUIT"

void each_client(SOCKET Connection,  SYNCHED_QUEUE *que) {
	int type;
	int len;
	char Buff[1024];

	printf("User (Socket : %d) is connected\n",  Connection);

	// GET IP ADDRESS
	SOCKADDR_IN temp_sock;
	int temp_sock_size = sizeof(temp_sock);
	getpeername(Connection, (SOCKADDR *)&temp_sock, &temp_sock_size);
	cout << "Connect IP : " << inet_ntoa(temp_sock.sin_addr) << endl;

	do
	{
		auto ret = recv(Connection, (char *)&type, sizeof(int), 0);
		if (ret != sizeof(int) || (type == DISCONN) ) // 비정상적인 종료의 경우
		{
//			printf("User %d(Socket : %d) is disconnected\n", User_ID, Connection);
			char *pBuf = Buff;
			memcpy(pBuf, &Connection, sizeof(SOCKET));
			pBuf += sizeof(SOCKET);
			memcpy(pBuf, "BYE SERVER!", sizeof("BYE SERVER!"));
			pBuf += sizeof("BYE SERVER!");
			*pBuf = '\0';

			len = pBuf - Buff;
			que->push(msg(DISCONN, len, Buff));
			
			break;

		}

		int SOCKET_SIZE = 0;
		char* pBuf = Buff;

		if (type == CONNECT)
		{
			SOCKET_SIZE = sizeof(SOCKET) / sizeof(char);

			memcpy(pBuf, &Connection, sizeof(SOCKET));
			pBuf += sizeof(SOCKET);
		}
		recv(Connection, (char *)&len, sizeof(int), 0);
		
		int inc = 0;
		do
		{
			auto ret = recv(Connection, pBuf, len-inc, 0);

			if (ret == EOF || ret == SOCKET_ERROR)
			{
				// 클라이언트가 죽었습니다. ㅠㅠ
				return;
			}
			pBuf += ret;
			inc += ret;
		} while (inc < len);

		if (inc > len)
		{
			// 패킷이... 이상한데?
			return;
		}
		//auto ret = recv(Connection, pBuf, len, 0);
		
		*pBuf = '\0';
		len += SOCKET_SIZE;

		int temp_sock;
		
		pBuf = Buff;
		memcpy(&temp_sock, Buff, sizeof(SOCKET));
		pBuf += sizeof(SOCKET);

		que->push(msg(type,len,Buff));
	} while (true);

	closesocket(Connection); // 연결 소켓을 닫는다.
}