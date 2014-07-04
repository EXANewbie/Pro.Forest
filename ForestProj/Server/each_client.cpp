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
		int ret = recv(Connection, (char *)&type, sizeof(int), 0);
		if (ret != sizeof(int)) // 비정상적인 종료의 경우
		{
			printf("User %d is disconnected", User_ID);
			break;
		}

		recv(Connection, (char *)&len, sizeof(int), 0);
		
		int USER_ID_SIZE = sizeof(int) / sizeof(char);
		for (int i = 0; i < USER_ID_SIZE; i++)
			Buff[i] = ((char *)&User_ID)[i];

			recv(Connection, Buff + USER_ID_SIZE, len, 0);
			
		Buff[len+USER_ID_SIZE] = '\0';
		len += USER_ID_SIZE;
		Buff[len] = '\0';

		// 유저가 종료 메시지를 전달한 경우

		int temp_user;
		
		for (int i = 0; i < 4; i++)
		{
			((char *)&temp_user)[i] = Buff[i];
		}
		
		

		printf("User %d : %s\n", temp_user, Buff+4);

		que->push(msg(type,len,Buff));
	} while (true);

	closesocket(Connection); // 연결 소켓을 닫는다.
}