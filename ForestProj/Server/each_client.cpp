#include <WinSock2.h>

#include <iostream>
#include <mutex>
#include <map>

#include "que.h"
#include "msg.h"

using namespace std;

#define END_MSG "\\QUIT"

void each_client(SOCKET Connection, int User_ID, SYNCHED_QUEUE *que) {

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
		int ret = recv(Connection, (char *)&len, sizeof(int), 0);
		if (ret != sizeof(int)) // 비정상적인 종료의 경우
		{
			printf("User %d is disconnected", User_ID);
			break;
		}
		recv(Connection, Buff, len, 0);
		Buff[len] = '\0';

		if (strcmp(Buff, END_MSG) == 0) // 유저가 종료 메시지를 전달한 경우
		{
			printf("User %d is quit", User_ID);
			break;
		}

		printf("User %d : %s\n", User_ID, Buff);

		que->push(msg(0,len,Buff));
	} while (true);

	closesocket(Connection); // 연결 소켓을 닫는다.
}