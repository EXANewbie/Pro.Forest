#include <WinSock2.h>
#include <string>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "Completion_Port.h"
#include "Memory_Pool.h"
#include "types.h"

#define PORT 78911

#define SERVER_IP_ADDRESS /*"localhost"*/"10.1.7.206"

ioInfo_Pool *ioInfo_Pool::instance;
Handler_Pool *Handler_Pool::instance;
Memory_Pool *Memory_Pool::instance;

std::mutex ioInfo_Pool::mtx;
std::mutex Handler_Pool::mtx;
std::mutex Memory_Pool::mtx;

void copy_to_buffer(char *pBuf, int* type, int* len, std::string* content);
void printLog(const char *msg, ...);
void sender(std::vector<handledata>* HandleVector);

unsigned WINAPI Client_Bot_Worker(LPVOID);

void main(void)
{
	std::vector<handledata> HandleVector;
	srand((unsigned int)time(NULL));
	
	auto HandlerPool = Handler_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();

	WSADATA wsaData;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;
	HANDLE hComPort;

	SOCKADDR_IN ServerAddr;
	SOCKADDR_IN ClientAddr;
	DWORD recvBytes, flags = 0;

	// 윈속 2.2로 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NULL) {
		printLog("WASStartup failed with error \n");
		return;
	}

	// CP 생성
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// 시스템 정보 불러오기
	GetSystemInfo(&sysInfo);

	// 시스템의 수만큼 스레드를 생성하여 CP에 등록
	for (int i = 0; i < 2 * sysInfo.dwNumberOfProcessors/*1*/; ++i)
	{
		_beginthreadex(NULL, 0, Client_Bot_Worker, (LPVOID)hComPort, 0, 0);
	}

	printf("how many client?\n");
	int numSock;
	scanf_s("%d", &numSock);

	SOCKET ListeningSocket;

	while (numSock--)
	{
		// 클라이언트용 소켓 생성
		if ((ListeningSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
		{
			printLog("WSASocket failed with error \n");
			return;
		}

		memset(&ServerAddr, 0, sizeof(ServerAddr));
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(PORT);
		ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

		// 서버에 연결
		if (connect(ListeningSocket, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
		{
			printLog("connect failed with error \n");
		}
		printLog("connection success\n");


		// 데이터 송신 부분
		handleInfo = HandlerPool->popBlock();
		handleInfo->hClntSock = ListeningSocket;
		handleInfo->char_id = -13;

		int ClientAddrLen = sizeof(ClientAddr); // 클라이언트 어드레스의 길이를 저장

		memcpy(&(handleInfo->clntAdr), &ClientAddr, ClientAddrLen);

		CreateIoCompletionPort((HANDLE)ListeningSocket, hComPort, (DWORD)handleInfo, 0);

		HandleVector.push_back(handledata(handleInfo));

		ioInfo = ioInfoPool->popBlock();
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->block = MemoryPool->popBlock();
		ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
		ioInfo->wsaBuf.len = BLOCK_SIZE;
		ioInfo->RWmode = READ;

		flags = 0;
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);

	}

	sender(&HandleVector);


}
