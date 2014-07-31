#include <iostream>
#include <set>

#include <process.h>
#include <WinSock2.h>
#include <Windows.h>

//#include "Client_Map.h"
#include "Check_Map.h"
#include "Completion_Port.h"
#include "Sock_set.h"
#include "Memory_Pool.h"
#include "TimerThread.h"

#include "DMap.h"

unsigned WINAPI Server_Worker(LPVOID);

Sock_set *Sock_set::instance;
ioInfo_Pool *ioInfo_Pool::instance;
Handler_Pool *Handler_Pool::instance;
Memory_Pool *Memory_Pool::instance;
F_Vector *F_Vector::instance;
Access_Map *Access_Map::instance;
Check_Map *Check_Map::instance;
Timer *Timer::instance;

using std::cout;
using std::endl;

//#define PACKET_SIZE_TEST

#ifdef PACKET_SIZE_TEST
void test(int);
#endif

void main() {
#ifdef PACKET_SIZE_TEST
	test(30000);
#endif
	Sock_set *Sock_set = Sock_set::getInstance();
	ioInfo_Pool *ioInfo_Pool = ioInfo_Pool::getInstance();
	Handler_Pool *Handler_Pool = Handler_Pool::getInstance();
	Memory_Pool *Memory_Pool = Memory_Pool::getInstance();
	Timer *timer = Timer::getInstance();

	F_Vector::makeThis();
	Access_Map::makeThis();

	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;
	HANDLE hComPort;
	
	SOCKET ListeningSocket;
	SOCKET NewConnection;
	SOCKADDR_IN ServerAddr;
	SOCKADDR_IN ClientAddr;
	DWORD recvBytes, flags = 0;

	int ClientAddrLen = sizeof(ClientAddr); // 클라이언트 어드레스의 길이를 저장

	// 윈도우 소켓 2.2로 초기화
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NULL ) {
		printLog("WASStartup failed with error \n");
		return;
	}

	// CP 생성
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	timer->setCompletionPort(hComPort);
	timer->start();

	// 시스템 정보 불러오기
	GetSystemInfo(&sysInfo);

	// 시스템의 수만큼 스레드를 생성하여 CP에 등록
	for (int i = 0; i <1/*2*sysInfo.dwNumberOfProcessors*/; ++i)
	{
		_beginthreadex(NULL, 0, Server_Worker, (LPVOID)hComPort, 0, 0);
	}

	// 연결을 기다리기 위한 소켓 생성
	if ((ListeningSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printLog("WSASocket failed with error \n");
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
		printLog("Bind failed with error \n");
		return;
	}

	// 클라이언트의 연결을 기다림
	// backlog는 일반적으로 5
	if (listen(ListeningSocket, 5000) == SOCKET_ERROR )
	{
		//printLog("Listen failed with error \n");
		return;
	}

	
	//새로운 연결을 하나 수락
	int cnt = 0;
	while (true) {
		if (cnt >= 16360) {
			printf("hello\n");
		}
		if( (NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen)) == SOCKET_ERROR )
		{
			printf("Socket failed with error(%d)\n", WSAGetLastError());
//			printLog("Socket failed with error(%d)\n", WSAGetLastError());
		}
		printLog("User (Socket : %d) is connected\n", NewConnection);

		handleInfo = Handler_Pool->popBlock();
		handleInfo->hClntSock = NewConnection;
		

		memcpy(&(handleInfo->clntAdr), &ClientAddr, ClientAddrLen);

		CreateIoCompletionPort((HANDLE)NewConnection, hComPort, (DWORD)handleInfo,0);

		//ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		ioInfo = ioInfo_Pool->popBlock();
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = HEADER_SIZE;	/* 이 부분에서 BUFFER_SIZE를 필히 수정해야됨, (엄밀히 말하면 8바이트만 먼저 받도록)*/
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->RWmode = READ;
		ioInfo->id = NOT_JOINED;
		ioInfo->myCharacter = NULL;
		ioInfo->type = UNDEFINED;
		ioInfo->len = UNDEFINED;
		ioInfo->offset = UNDEFINED;
		ioInfo->block = nullptr;
				
		Sock_set->insert(NewConnection);
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);

		// GET IP ADDRESS
		SOCKADDR_IN temp_sock;
		int temp_sock_size = sizeof(temp_sock);
		getpeername(NewConnection, (SOCKADDR *)&temp_sock, &temp_sock_size);
		//cout << "Connect IP : " << inet_ntoa(temp_sock.sin_addr) << endl;
		printLog("Connect IP : %s\n", inet_ntoa(temp_sock.sin_addr));
		cnt++;
	}
	 
	closesocket(ListeningSocket); // 리스닝 소켓을 닫는다.
}

#ifdef PACKET_SIZE_TEST

#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"

void test(int n) {

	ERASE_USER::CONTENTS eraseuserContent;

	for (int i = 0; i < n; i++) {
		auto data = eraseuserContent.add_data();
		data->set_id(i);
	}
	{
		std::string byteString;
		eraseuserContent.SerializeToString(&byteString);
		printLog("eraseuser's size in %d users is %d byte", n, byteString.length());
	}

	INIT::CONTENTS initContent;

	for(int i = 0; i < n; i++) {
		auto data = initContent.add_data();
		data->set_id(i);
		data->set_x(i);
		data->set_y(0x7fffffff);
	}
	{
		std::string byteString;
		initContent.SerializeToString(&byteString);
		printLog("init's size in %d users is %d byte", n, byteString.length());
	}
}

#endif
