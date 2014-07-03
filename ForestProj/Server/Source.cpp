#include <cstdio>
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <map>

#include "que.h"
#include "msg.h"

void each_client(SOCKET, int, SYNCHED_QUEUE *);
void sender(SYNCHED_QUEUE *, std::map<int, SOCKET> *);

void main() {
	WSADATA wasData;
	SOCKET ListeningSocket;
	SOCKET NewConnection;
	SOCKADDR_IN ServerAddr;
	SOCKADDR_IN ClientAddr;
	int Port = 78911;

	// 윈도우 소켓 2.2로 초기화
	int Ret;
	if ((Ret = WSAStartup(MAKEWORD(2, 2), &wasData)) != 0) {
		printf("WASStartup failed with error %d\n", Ret);
		return;
	} // 연결을 기다리기 위한 소켓 생성

	ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListeningSocket == INVALID_SOCKET)
	{
		printf("error %d\n", WSAGetLastError());
		return;
	}

	// bind하기 위해 SOCKADDR_IN에 포트번호 Port와 INADDR_ANY로 설정
	// 주의, 호스트 바이트 순서는 네트웍 바이트 순서로 바꿔야한다!

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind를 이용하여 사용한 주소 지정

	int tmp = bind(ListeningSocket, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));

	if (tmp == SOCKET_ERROR)
	{
		printf("tmp : %d %d %d %d\n", tmp, WSAGetLastError(), WSAEFAULT, WSAEADDRINUSE);
	}

	// 클라이언트의 연결을 기다림
	// backlog는 일반적으로 5

	listen(ListeningSocket, 5);

	
	//새로운 연결을 하나 수락
	int ClientAddrLen = sizeof(ClientAddr); // 클라이언트 어드레스의 길이를 저장

	std::map<int, int> users;
	std::vector<std::thread> vec;
	std::map<int, SOCKET> *socks = new std::map<int, SOCKET>();
	SYNCHED_QUEUE *que = new SYNCHED_QUEUE();

	std::thread t1(sender, que, socks);

	int User_id = 1;

	while (true) {
		NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen);

//		socks_mtx.lock();
		(*socks)[User_id] = NewConnection;
//		socks_mtx.unlock();

		vec.push_back(std::thread(each_client, NewConnection, User_id, que));

		User_id++;
	}

	closesocket(ListeningSocket); // 리스닝 소켓을 닫는다.

	// 클라이언트의 연결을 기다림

	if (WSACleanup() == SOCKET_ERROR) {
		printf("WASCleanup failed with error %d\n", WSAGetLastError());
	}
}