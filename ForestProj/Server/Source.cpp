#include <cstdio>
#include <cstdio>
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <set>

#include "que.h"
#include "msg.h"
#include "Client_Map.h"
#include "cmap.h"
#include "Disc_user_map.h"
#include "Synched_list.h"

void each_client();
void sender(std::set<SOCKET> *);
Client_Map *Client_Map::instance;
SYNCHED_QUEUE *SYNCHED_QUEUE::instance;
Disc_User_Map *Disc_User_Map::instance;
Synched_List *Synched_List::instance;

std::mutex Client_Map::mtx;
std::mutex SYNCHED_QUEUE::mtx;
std::mutex Disc_User_Map::mtx;
std::mutex Synched_List::mtx;

using std::cout;
using std::endl;

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

	std::set<SOCKET> *sock_set = new std::set<SOCKET>();

	std::thread t1(sender, sock_set);
	std::thread t2(each_client); // each_client -> receiver

	u_long ul = 1;

	Synched_List *List = Synched_List::getInstance();

	while (true) {
		NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen);

		printf("User (Socket : %d) is connected\n", NewConnection);

		// GET IP ADDRESS
		SOCKADDR_IN temp_sock;
		int temp_sock_size = sizeof(temp_sock);
		getpeername(NewConnection, (SOCKADDR *)&temp_sock, &temp_sock_size);
		cout << "Connect IP : " << inet_ntoa(temp_sock.sin_addr) << endl;

		bool opt = TRUE;
		setsockopt(NewConnection, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, sizeof(opt));
		ioctlsocket(NewConnection, FIONBIO, &ul); // set Non-Blocking Socket

		List->push_back(NewConnection);

		sock_set->insert(NewConnection);
	}
	 
	closesocket(ListeningSocket); // 리스닝 소켓을 닫는다.
}