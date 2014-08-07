﻿#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <conio.h>
#include <string>
#include <iostream>

#include "character.h"
#include "cmap.h"
#include "types.h"
#include "Scoped_Lock.h"
#include "mmap.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/setmonster.pb.h"
#include "../protobuf/erasemonster.pb.h"

#define PORT 78911

#define SERVER_IP_ADDRESS /*"localhost"*/"10.1.7.206"

SYNCHED_CHARACTER_MAP* SYNCHED_CHARACTER_MAP::instance;
SYNCHED_MONSTER_MAP* SYNCHED_MONSTER_MAP::instance;

void send_move(const SOCKET s,const char& c,const int& myID);
void copy_to_buffer(char *buf, int* type, int* len, std::string* content);
void receiver(const SOCKET s, int* myID, Character* myChar);
void Sender(const SOCKET sock, int* myID, Character* myChar);

void main(void)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	WSADATA wsaData;
	SOCKET s;
	SOCKADDR_IN ServerAddr;

	// 윈속 2.2로 초기화
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 클라이언트용 소켓 생성
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

	// 서버에 연결
	if (connect(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr)) == -1)
	{
		printf("연결실패\n");
	}
	
	printf("                              --MINI EXA GAME--                              \n");
	printf("본 게임은 몬스터를 사냥해서 레벨업을 하여 성장시키는 MMORPG 게임입니다.\n");
	printf("재미나게 즐겨주시기 바랍니다^^\n\n");
	Sleep(1000);
	//이름 입력받기.
	printf("-당신의 이름은 무엇입니까?\n");
	std::string name;
	std::cin >> name;

	// 데이터 송신 부분
	int type;
	int len;
	char *buf = new char[1024000] ; 

	//PCONNECT 전송.
	type = PCONNECT;
	
	char* pBuf = buf;
	
	CONNECT::CONTENTS contents;
	std::string* buff_msg = contents.mutable_data();
	*(contents.mutable_name()) = name;
	*buff_msg = "HELLO SERVER!";
	
/*	for (int i = 0; i < 5000; i++)		//패킷 사이즈를 약 50kb로 쐇을 때 끊겨오는 현상 발견, 테스트용으로 넣음
		(*buff_msg).append(" Hi Hello!");*/ // 만약 이 테스트를 하고 싶다면 BLOCK_SIZE = 1 << 17;로 수정 권고
	std::string bytestring;
	contents.SerializeToString(&bytestring);
	
	len = bytestring.length();
	
	copy_to_buffer(buf, &type, &len, &bytestring);
	
	Sleep(500);
	printf("※주의 : 조작은 영어로만 가능합니다. 만일 한글키로 되어있을 경우 한영키를 눌러 영어로 바꾸세요:)\n");
	Sleep(1000);
	printf("사냥터접속중.......\n");
	Sleep(3000);
	printf("사냥터접속 성공\n");
	printf("            ★-----소환사의 협곡에 오신 것을 환영합니다-----★\n\n");

	send(s, buf, len + sizeof(int) * 2, 0);
	
	//자신의 캐릭터 생성
	int myID;
	Character myChar(-1,-1);

	// 데이터 수신 쓰레드 동작.
	std::thread t(receiver, s, &myID, &myChar);
	
	Sender(s,&myID,&myChar);

	t.join();
	closesocket(s);
}

