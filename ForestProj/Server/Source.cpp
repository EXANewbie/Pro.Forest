#include <iostream>
#include <set>

#include <process.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Client_Map.h"
#include "Completion_Port.h"
#include "Sock_set.h"
#include "Memory_Pool.h"

unsigned WINAPI Server_Worker(LPVOID);

Client_Map *Client_Map::instance;
Sock_set *Sock_set::instance;
ioInfo_Pool *ioInfo_Pool::instance;
Handler_Pool *Handler_Pool::instance;
Memory_Pool *Memory_Pool::instance;

std::mutex Client_Map::mtx;
std::mutex Sock_set::mtx;
std::mutex ioInfo_Pool::mtx;
std::mutex Handler_Pool::mtx;
std::mutex Memory_Pool::mtx;

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

	// 시스템 정보 불러오기
	GetSystemInfo(&sysInfo);

	// 시스템의 수만큼 스레드를 생성하여 CP에 등록
	for (int i = 0; i <2*sysInfo.dwNumberOfProcessors/*1*/; ++i)
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
	listen(ListeningSocket, 200);

	
	//새로운 연결을 하나 수락
	Sock_set *sock_set = Sock_set::getInstance();
	auto HandlerPool = Handler_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();

	while (true) {
		NewConnection = accept(ListeningSocket, (SOCKADDR *)&ClientAddr, &ClientAddrLen);
		printLog("User (Socket : %d) is connected\n", NewConnection);

		handleInfo = HandlerPool->popBlock();
		handleInfo->hClntSock = NewConnection;
		

		memcpy(&(handleInfo->clntAdr), &ClientAddr, ClientAddrLen);

		CreateIoCompletionPort((HANDLE)NewConnection, hComPort, (DWORD)handleInfo,0);

		//ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		ioInfo = ioInfoPool->popBlock();
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
				
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
		sock_set->insert(NewConnection);

		// GET IP ADDRESS
		SOCKADDR_IN temp_sock;
		int temp_sock_size = sizeof(temp_sock);
		getpeername(NewConnection, (SOCKADDR *)&temp_sock, &temp_sock_size);
		//cout << "Connect IP : " << inet_ntoa(temp_sock.sin_addr) << endl;
		printLog("Connect IP : %s\n", inet_ntoa(temp_sock.sin_addr));

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
