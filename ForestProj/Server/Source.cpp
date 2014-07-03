#include <cstdio>
#include <WinSock2.h>
#include <iostream>

using namespace std;

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
	/*
	if (bind(server, (sockaddr *)&sin, sizeof(sin) == SOCKET_ERROR))
	{
	wError = WSAGetLastError();
	cerr << "Error: bin() return value == SOCKET_ERROR\n"
	"Details: " << wError << endl;
	WSACleanup();
	exit(EXIT_FAILURE);
	}*/
	if (tmp == SOCKET_ERROR)
	{
		printf("tmp : %d %d %d %d\n", tmp, WSAGetLastError(), WSAEFAULT, WSAEADDRINUSE);
	}

	// 클라이언트의 연결을 기다림
	// backlog는 일반적으로 5

	listen(ListeningSocket, 5);

	printf("%d\n", WSAGetLastError());

	// 새로운 연결을 하나 수락
	int ClientAddrLen = sizeof(ClientAddr); // 클라이언트 어드레스의 길이를 저장
	NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen);

	// 여기에 할 일을 적는다.

	cout << "연결이 성공하였습니다." << endl;

	SOCKADDR_IN temp_sock;
	int temp_sock_size = sizeof(temp_sock);

	getpeername(NewConnection, (SOCKADDR *)&temp_sock, &temp_sock_size);
	
	cout << inet_ntoa(temp_sock.sin_addr) << endl;

	while (true)
	{
		char buffer[1024];
		int len;
		if (recv(NewConnection, (char*)&len, sizeof(int), 0) != sizeof(int))
			break;
		int ret = recv(NewConnection, buffer, len, 0);
		buffer[len] = '\0';
		if (strcmp(buffer, "end") == 0)
		{
			strncpy_s(buffer, "Server is closed", sizeof("Server is closed"));
			len = strlen(buffer);
			send(NewConnection, (char *)&len, sizeof(int), 0);
			send(NewConnection, buffer, len, 0);
			break;
		}
		if (ret != len)
		{
			cout << "disconnected" << endl;
			break;
		}
		cout << buffer << ", len " << ret << endl;

	}

	closesocket(NewConnection); // 연결 소켓을 닫는다.
	closesocket(ListeningSocket); // 리스닝 소켓을 닫는다.

	// 클라이언트의 연결을 기다림

	if (WSACleanup() == SOCKET_ERROR) {
		printf("WASCleanup failed with error %d\n", WSAGetLastError());
	}
}