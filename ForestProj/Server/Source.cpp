#include <iostream>
#include <set>

#include <process.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Client_Map.h"
#include "Completion_Port.h"
#include "Sock_set.h"

unsigned WINAPI Server_Worker(LPVOID);
CRITICAL_SECTION cs;

Client_Map *Client_Map::instance;
Sock_set *Sock_set::instance;

std::mutex Client_Map::mtx;
std::mutex Sock_set::mtx;

using std::cout;
using std::endl;

void main() {
	
	WSADATA wasData;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;
	HANDLE hComPort;
	
	SOCKET ListeningSocket;
	SOCKET NewConnection;
	SOCKADDR_IN ServerAddr;
	SOCKADDR_IN ClientAddr;
	DWORD recvBytes, flags = 0;

	InitializeCriticalSection(&cs);

	int ClientAddrLen = sizeof(ClientAddr); // 클라이언트 어드레스의 길이를 저장

	// 윈도우 소켓 2.2로 초기화
	
	if (WSAStartup(MAKEWORD(2, 2), &wasData) != NULL ) {
		printf("WASStartup failed with error \n");
		return;
	}

	// CP 생성
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// 시스템 정보 불러오기
	GetSystemInfo(&sysInfo);

	// 시스템의 수만큼 스레드를 생성하여 CP에 등록
	for (int i = 0; i </*2*sysInfo.dwNumberOfProcessors*/1; ++i)
	{
		_beginthreadex(NULL, 0, Server_Worker, (LPVOID)hComPort, 0, 0);
	}

	// 연결을 기다리기 위한 소켓 생성
	if ((ListeningSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("WSASocket failed with error \n");
		return;
	}

	// bind하기 위해 SOCKADDR_IN에 포트번호 Port와 INADDR_ANY로 설정
	// 주의, 호스트 바이트 순서는 네트웍 바이트 순서로 바꿔야한다!
	memset(&ServerAddr, 0, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind를 이용하여 사용한 주소 지정
	if (bind(ListeningSocket, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
	{
		printf("Bind failed with error \n");
		return;
	}

	// 클라이언트의 연결을 기다림
	// backlog는 일반적으로 5
	listen(ListeningSocket, 5);

	
	//새로운 연결을 하나 수락

	std::set<SOCKET> *sock_set = new std::set<SOCKET>();

	while (true) {
		NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen);
		printf("User (Socket : %d) is connected\n", NewConnection);

		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDEL_DATA));
		handleInfo->hClntSock = NewConnection;
		memcpy(&(handleInfo->clntAdr), &ClientAddr, ClientAddrLen);

		CreateIoCompletionPort((HANDLE)NewConnection, hComPort, (DWORD)handleInfo,0);

		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUFFER_SIZE;	/* 이 부분에서 BUFFER_SIZE를 필히 수정해야됨, (엄밀히 말하면 8바이트만 먼저 받도록)*/
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->RWmode = READ;
		ioInfo->id = NOT_JOINED;
		ioInfo->myCharacter = NULL;
				
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
		sock_set->insert(NewConnection);

		// GET IP ADDRESS
		SOCKADDR_IN temp_sock;
		int temp_sock_size = sizeof(temp_sock);
		getpeername(NewConnection, (SOCKADDR *)&temp_sock, &temp_sock_size);
		cout << "Connect IP : " << inet_ntoa(temp_sock.sin_addr) << endl;

	}
	 
	DeleteCriticalSection(&cs);
	closesocket(ListeningSocket); // 리스닝 소켓을 닫는다.
}