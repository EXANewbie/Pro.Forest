#include <cstdio>
#include <vector>

#include "Completion_Port.h"
#include "types.h"
#include "Disc_user_map.h"
#include "Client_Map.h"

using std::vector;

void set_single_cast(int , vector<SOCKET>& );
void set_multicast_in_room_except_me(int id, vector<SOCKET>& send_list);
void send_message(char*, int, vector<SOCKET> &);

unsigned WINAPI Server_Worker(LPVOID pComPort)
{
	Client_Map *CMap = Client_Map::getInstance();
	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();

	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	vector<SOCKET> receiver;

	while (true)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		puts("MESSAGE RECEIVED!");
		if ( bytesTrans == 0) // 올바르지 않은 종류의 경우
		{
			closesocket(sock);
			free(handleInfo); free(ioInfo);
			continue;
		}

		//메시지를 받은 후 처리해야할 계산 부분

		int readbyte = 0;
		int type, len;
		char buf[BUFFER_SIZE];

		memcpy(&type, ioInfo->buffer + readbyte, sizeof(int));
		readbyte += sizeof(int);
		memcpy(&len, ioInfo->buffer + readbyte, sizeof(int));
		readbyte += sizeof(int);

		if (type == DISCONN) // 정상적인 종료의 경우
		{
			memcpy(buf, ioInfo->buffer + readbyte, len);
			if (strcmp(DSCMSG, buf))
			{
				//가짜 클라이언트
			}

			CMap->lock();
			CMap->erase(sock);
			CMap->unlock();
						
			closesocket(sock); // 연결 소켓을 닫는다.

		}
		else if (type == CONNECT) // 새로 들어온 경우
		{
			memcpy(buf, ioInfo->buffer + readbyte, len);
			if (strcmp(CONMSG, buf))
			{
				//가짜 클라이언트
			}


			int char_id;
			int x, y;
			static int id = 1;
			int writebyte;

			// 캐릭터 객체를 생성 후
			Character c(id);
			char_id = id;
			x = c.getX();
			y = c.getY();
			id++;

			
			sock_set->erase(sock);
			CMap->lock();
			CMap->insert(char_id, sock, c);
			CMap->unlock();
						
			// x와 y의 초기값을 가져온다.	
			type = INIT;
			len = 3 * sizeof(int);

			writebyte = 0;

			for (int i = 0; i < 5; i++)
			{
				int *param[] = { &type, &len, &char_id, &x, &y };
				memcpy(buf + writebyte, param[i], sizeof(int));
				writebyte += sizeof(int);
			}

			set_single_cast(char_id, receiver);
			send_message(buf, writebyte, receiver);
			receiver.clear();

			// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.

			type = SET_USER;
/*			len = 3 * sizeof(int);

			for (int i = 0; i < 5; i++)
			{
				int *param[] = { &type, &len, &char_id, &x, &y };
				memcpy(buf + writebyte, param[i], sizeof(int));
				writebyte += sizeof(int);
			}
			*/
			set_multicast_in_room_except_me(char_id, receiver);
			send_message(buf, writebyte, receiver);
			receiver.clear();

			// CONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.

			type = SET_USER;
			len = 0;

			writebyte = sizeof(int) * 2;

			CMap->lock();
			for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
			{
				const int tID = itr->second.getID();
				const int tx = itr->second.getX();
				const int ty = itr->second.getY();

				if (tID == char_id) // 캐릭터 맵에 현재 들어간 내 객체의 정보를 보내려 할 때는 건너뛴다.
					continue;

				if (tx == x && ty == y)
				{
					for (int i = 0; i < 3; i++)
					{
						const int *param[] = { &tID, &tx, &ty };
						memcpy(buf + writebyte, param[i], sizeof(int));
						writebyte += sizeof(int);
					}
					len += 3 * sizeof(int);
				}
			}
			CMap->unlock();

			writebyte = 0;
			for (int i = 0; i < 2; i++)
			{
				const int *param[] = { &type, &len };
				memcpy(buf + writebyte, param[i], sizeof(int));
				writebyte += sizeof(int);
			}

			writebyte += len;

			set_single_cast(char_id, receiver);
			send_message(buf, writebyte, receiver);
			receiver.clear();	
		}
		else // 나머지 문자의 경우
		{
			for (int inc = 0; inc < sizeof(int);)
			{
				int ret = recv(Connection, (char *)&len, sizeof(int), 0);
				if (ret != SOCKET_ERROR)
				{
					inc += ret;
				}
			}
			char* pBuf = Buff;
			printf("len : %d\n", len);
			for (int inc = 0; inc < len;)
			{
				int ret = recv(Connection, pBuf + inc, len - inc, 0);
				if (ret != SOCKET_ERROR)
				{
					inc += ret;
				}
			}

			que->push(msg(type, len, Buff));
			itr++;
		}
		/*SEND가 처리해야될 부분
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = bytesTrans;
		WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, NULL, NULL);
		*/ 

		// 다시 받을 준비를 하는 부분
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUFFER_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
	}

	return 0;
}

void set_single_cast(int id, vector<SOCKET>& send_list)
{
	Client_Map *CMap = Client_Map::getInstance();
	CMap->lock();
	SOCKET sock = CMap->find_id_to_sock(id);
	CMap->unlock();
	send_list.push_back(sock);
}

void set_multicast_in_room_except_me(int id, vector<SOCKET>& send_list)
{
	Client_Map *CMap = Client_Map::getInstance();

	CMap->lock();

	auto now = CMap->find_id_to_char(id);

	for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
	{
		if (now->getX() == itr->second.getX() && now->getY() == itr->second.getY())
		{
			if (now->getID() != itr->second.getID())
			{
				auto sock = CMap->find_id_to_sock(itr->first);
				send_list.push_back(sock);
			}
		}
	}
	CMap->unlock();
}

void send_message(char* msg, int len, vector<SOCKET> &send_list) {
	Client_Map *CMap = Client_Map::getInstance();
	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();
	//vector< pair<int,SOCKET> > errors;

	WSABUF wsabuf;
	wsabuf.buf = msg;
	wsabuf.len = len;

	for (int i = 0; i < send_list.size(); i++)
	{
		SOCKET sock = send_list[i];

		WSASend(sock, &wsabuf, 1, NULL, 0, NULL, NULL);

		/*
		char* pBuf = buf;
		int tlen = 0;
		memcpy(pBuf, &message.type, sizeof(int));
		pBuf += sizeof(int);
		tlen += sizeof(int);
		memcpy(pBuf, &message.len, sizeof(int));
		pBuf += sizeof(int);
		tlen += sizeof(int);
		memcpy(pBuf, message.buff, message.len);
		tlen += message.len;
		*/

		//		for (int inc = 0; inc < tlen;)
		//		{
		//		int ret = send(sock, buf/*+inc*/, tlen/*-inc*/, 0);
		//			if (ret != SOCKET_ERROR)
		//			{
		//				inc += ret;
		//			}
		//		}

		/*int ret = send(sock, (char *)&message.type, sizeof(int), 0);
		if (ret != sizeof(int))
		{
		auto it = Disc_User->find(sock);
		if (it == Disc_User->end())
		{
		CMap->lock();
		int erase_id = CMap->find_sock_to_id(sock);
		CMap->unlock();
		Disc_User->insert(pair<SOCKET, int>(sock, erase_id));
		que->push(msg(DISCONN, sizeof(SOCKET), (char *)&sock));
		//(*Disc_User)[sock] = erase_id;
		}
		continue;
		}
		send(sock, (char *)&message.len, sizeof(int), 0);
		send(sock, (char *)&message.buff, message.len, 0);*/
	}

	// 전송 실패한 유저들을 모아서 전송

	//if (!errors.empty())
	//{
	//send_erase_user_message(CMap, errors);
	//}
}