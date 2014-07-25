#include <memory>
#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>
#include <string>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "character.h"
#include "Completion_Port.h"
#include "Memory_Pool.h"
#include "types.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"

#define PORT 78911

#define SERVER_IP_ADDRESS /*"localhost"*/"10.1.7.10"


ioInfo_Pool *ioInfo_Pool::instance;
Handler_Pool *Handler_Pool::instance;
Memory_Pool *Memory_Pool::instance;

std::mutex ioInfo_Pool::mtx;
std::mutex Handler_Pool::mtx;
std::mutex Memory_Pool::mtx;

void send_move(const SOCKET s,const char& c,const int& myID);
void copy_to_buffer(char *buf, int* type, int* len, std::string* content);

void printLog(const char *msg, ...)
{
#ifdef PRINT_LOG
	const int BUF_SIZE = 512;
	char buf[BUF_SIZE] = { 0, };
	va_list ap;

	strcpy_s(buf, "Log : ");
	va_start(ap, msg);
	vsprintf_s(buf + strlen(buf), BUF_SIZE - strlen(buf), msg, ap);
	va_end(ap);

	puts(buf);
#endif;
}


unsigned WINAPI Client_Bot_Worker(LPVOID);

struct handleioInfo
{
	LPPER_HANDLE_DATA handleInfo;

	int tic;
	int state;
	handleioInfo(LPPER_HANDLE_DATA handleInfo)
	{
		this->handleInfo = handleInfo;
		tic = (rand()%9901)+100;
		state = PCONNECT;
	}
};

std::vector<handleioInfo> vh;

void main(void)
{
	srand((unsigned int)time(NULL));
	//auto chars = SYNCHED_CHARACTER_MAP::getInstance();
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

		vh.push_back(handleioInfo(handleInfo));

		ioInfo = ioInfoPool->popBlock();
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->block = MemoryPool->popBlock();
		ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
		ioInfo->wsaBuf.len = BLOCK_SIZE;
		ioInfo->RWmode = READ;
		flags = 0;
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);

	}

	//만약 작동을 하라고 명령을 내리면
	//sender 쓰레드 생성 (자동 시작)

	//보낸다......

	int dxy[4][2]{{ -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }};

	while (true)
	{
		LPPER_IO_DATA ioInfo;
		//LPPER_HANDLE_DATA handleInfo;

		for (int i = 0; i < vh.size(); ++i)
		{
			//printf("!!\n");
			vh[i].tic -= 1;
			if (vh[i].tic == 0)
			{
				int randInt = (rand() %1801) + 200;
				vh[i].tic = randInt;

				if (vh[i].state == PCONNECT)
				{
					vh[i].state = PINIT;
					int type = PCONNECT;

					std::string bytestring;
					CONNECT::CONTENTS contents;
					std::string* buff_msg = contents.mutable_data();
					*buff_msg = "HELLO SERVER!";

					contents.SerializeToString(&bytestring);
					int len = bytestring.length();

					ioInfo = ioInfoPool->popBlock();
					memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));

					ioInfo->block = MemoryPool->popBlock();
					ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();

					copy_to_buffer(ioInfo->wsaBuf.buf, &type, &len, &bytestring);
					ioInfo->wsaBuf.len = len + sizeof(int) * 2;
					ioInfo->RWmode = WRITE;
					int ret = WSASend(vh[i].handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

					if (ret == SOCKET_ERROR)
					{
						if (WSAGetLastError() == ERROR_IO_PENDING)
						{
							//					printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
							// 큐에 들어감 ^.^
						}
						else
						{
							// 너에겐 수많은 이유가 있겠지... 하지만 아마도 그 수많은 이유들의 공통점은 소켓에 전송할 수 없는 것이 아닐까?
							if (ioInfo->block != nullptr) {
								MemoryPool->pushBlock(ioInfo->block);
							}
							ioInfoPool->pushBlock(ioInfo);
							//free(ioInfo);
						}
						printLog("Send Error (%d)\n", WSAGetLastError());
					}
					else
					{
						//				printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
					}
					contents.Clear();

					flags = 0;
					
					//ioInfo = ioInfoPool->popBlock();
					//memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
					//ioInfo->block = MemoryPool->popBlock();
					//ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
					//ioInfo->wsaBuf.len = BLOCK_SIZE;
					//ioInfo->RWmode = READ;
//					WSARecv(vh[i].handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
					
				}
				else
				{

					int type = PMOVE_USER;
					std::string bytestring;
					MOVE_USER::CONTENTS contents;

					if (vh[i].handleInfo->char_id == -13) continue;
					auto element = contents.mutable_data()->Add();
					element->set_id(vh[i].handleInfo->char_id);
					element->set_xoff(dxy[randInt % 4][0]);
					element->set_yoff(dxy[randInt % 4][1]);

					contents.SerializeToString(&bytestring);
					int len = bytestring.length();

					ioInfo = ioInfoPool->popBlock();
					memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));

					ioInfo->block = MemoryPool->popBlock();
					ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();

					copy_to_buffer(ioInfo->wsaBuf.buf, &type, &len, &bytestring);
					ioInfo->wsaBuf.len = len + sizeof(int)*2;
					ioInfo->RWmode = WRITE;
					WSASend(vh[i].handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
					contents.Clear();

					flags = 0;

					//ioInfo = ioInfoPool->popBlock();
					//memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
					//ioInfo->block = MemoryPool->popBlock();
					//ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
					//ioInfo->wsaBuf.len = BLOCK_SIZE;
					//ioInfo->RWmode = READ;
					//WSARecv(vh[i].handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
				}
			}
		}
		Sleep(1);
	}

}
void copy_to_buffer(char* pBuf, int* type, int* len, std::string* content)
{
	memcpy(pBuf, (char*)type, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, (char *)len, sizeof(int));
	pBuf += sizeof(int);
	memcpy(pBuf, content->c_str(), *len);
}