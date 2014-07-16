#include <cstdio>
#include <vector>

#include "Completion_Port.h"
#include "types.h"
#include "Disc_user_map.h"
#include "Client_Map.h"
#include "msg.h"
#include "Sock_set.h"

#include <Windows.h>

using std::vector;

void set_single_cast(int , vector<SOCKET>& );
void set_multicast_in_room_except_me(int, vector<SOCKET>&,bool);
void send_message(msg , vector<SOCKET> &);
void unpack(msg, char *, int *);
void closeClient(int);

extern CRITICAL_SECTION cs;
int k;

unsigned WINAPI Server_Worker(LPVOID pComPort)
{
	Client_Map *CMap = Client_Map::getInstance();
	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();
	Sock_set *sock_set = Sock_set::getInstance();

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

		if (ioInfo->RWmode == READ)
		{
			puts("MESSAGE RECEIVED!");
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printf("나옴ㅋ\n");
				CMap->lock();
				int char_id = CMap->find_sock_to_id(sock);
				// 아이디가 비어있는 경우
				if (char_id == -1)
				{
					// 이미 삭제 처리 된 경우를 여기에 명시한다.
				}
				else
				{
					printf("sock : %d char_id : %d\n", sock, char_id);
					closeClient(sock);
					free(handleInfo); free(ioInfo);
				}
				CMap->unlock();
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
				if (strcmp("BYE SERVER!", buf))
				{
					//가짜 클라이언트
				}

				CMap->lock();
				int char_id = CMap->find_sock_to_id(sock);
				// 아이디가 비어있는 경우
				if (char_id == -1)
				{
					// 이미 삭제 처리 된 경우를 여기에 명시한다.
				}
				else
				{
					closeClient(sock);
					free(handleInfo); free(ioInfo);
				}
				CMap->unlock();
				continue;
			}
			else if (type == CONNECT) // 새로 들어온 경우
			{
				memcpy(buf, ioInfo->buffer + readbyte, len);
				if (strcmp("HELLO SERVER!", buf))
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

				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &char_id, &x, &y };
					memcpy(buf + writebyte, param[i], sizeof(int));
					writebyte += sizeof(int);
				}

				set_single_cast(char_id, receiver);
				send_message(msg(type, len, buf), receiver);
				receiver.clear();

				// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.

				type = SET_USER;

				set_multicast_in_room_except_me(char_id, receiver, true/*autolock*/ );
				send_message(msg(type, len, buf), receiver);
				receiver.clear();

				// CONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.

				type = SET_USER;
				len = 0;

				writebyte = 0;

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

				set_single_cast(char_id, receiver);
				send_message(msg(type, len, buf), receiver);
				receiver.clear();
			}
			else if (type == MOVE_USER)// 유저가 이동하는 경우
			{
				memcpy(buf, ioInfo->buffer + readbyte, len);

				int ntype, nlen;
				int cur_id, x_off, y_off;
				int cnt;
				char *pBuf;

				int writebyte;

				writebyte = 0;
				for (int i = 0; i < len; i += sizeof(int)* 3)
				{
					int *param[] = { &cur_id, &x_off, &y_off };
					for (int j = 0; j < 3; j++)
					{
						memcpy(param[j], buf + writebyte, sizeof(int));
						writebyte += sizeof(int);
					}

					CMap->lock();
					Character *now = CMap->find_id_to_char(cur_id);
					CMap->unlock();

					char nbuf[BUFFER_SIZE];
					pBuf = nbuf;

					// 기존의 방의 유저들의 정보를 삭제함
					cnt = 0;
					CMap->lock();
					for (auto iter = CMap->begin(); iter != CMap->end(); iter++)
					{
						if (iter->second.getX() == now->getX() && iter->second.getY() == now->getY())
						{
							int nid = iter->second.getID();

							if (nid == now->getID())
								continue;

							memcpy(pBuf, &nid, sizeof(int));
							pBuf += sizeof(int);
							cnt++;
						}
					}
					CMap->unlock();

					ntype = ERASE_USER;
					nlen = sizeof(int)*cnt;

					set_single_cast(now->getID(), receiver);
					send_message(msg(ntype, nlen, nbuf), receiver);
					receiver.clear();

					// 기존 방의 유저들에게 내가 사라짐을 알림
					set_multicast_in_room_except_me(now->getID(), receiver, true/*autolock*/ );
					send_message(msg(ntype, sizeof(int), (char *)&cur_id), receiver);
					receiver.clear();

					// 캐릭터를 해당 좌표만큼 이동시킴
					now->setX(now->getX() + x_off);
					now->setY(now->getY() + y_off);

					// 새로운 방의 유저들에게 내가 등장함을 알림
					int id = now->getID(), x = now->getX(), y = now->getY();
					pBuf = nbuf;
					for (int i = 0; i < 3; i++)
					{
						int *param[] = { &id, &x, &y };
						memcpy(pBuf, param[i], sizeof(int));
						pBuf += sizeof(int);
					}

					set_multicast_in_room_except_me(now->getID(), receiver, true/*autolock*/ );
					send_message(msg(SET_USER, 3 * sizeof(int), nbuf), receiver);
					receiver.clear();

					// 새로운 방의 유저들의 정보를 불러옴
					pBuf = nbuf;
					cnt = 0;
					CMap->lock();
					for (auto iter = CMap->begin(); iter != CMap->end(); iter++)
					{
						if (iter->second.getX() == x && iter->second.getY() == y)
						{
							int nid = iter->second.getID();
							int nx = iter->second.getX();
							int ny = iter->second.getY();
							for (int i = 0; i < 3; i++)
							{
								int *param[] = { &nid, &nx, &ny };
								memcpy(pBuf, param[i], sizeof(int));
								pBuf += sizeof(int);
							}
							cnt++;
						}
					}
					CMap->unlock();
					set_single_cast(now->getID(), receiver);
					send_message(msg(SET_USER, 3 * sizeof(int)* cnt, nbuf), receiver);
					receiver.clear();

					printf("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);
				}
			}
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUFFER_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->RWmode = READ;

			int ret = WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);

			if (ret == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSA_IO_PENDING )
				{

				}
				else
				{
					// 소켓 에러 발생
				}
			}
		}
		else // WRITE
		{
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printf("나감ㅋ\n");
				closeClient(sock);
				free(handleInfo); free(ioInfo);
				continue;
			}

			puts("MESSAGE SEND!");
			free(ioInfo);
			printf("k Decrement %d\n", InterlockedDecrement((unsigned int *)&k));
		}
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

void set_multicast_in_room_except_me(int id, vector<SOCKET>& send_list, bool autolocked)
{
	Client_Map *CMap = Client_Map::getInstance();

	if (autolocked == true)
	{
		CMap->lock();
	}

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

	if (autolocked == true)
	{
		CMap->unlock();
	}
}



void send_message(msg message, vector<SOCKET> &send_list) {
	Client_Map *CMap = Client_Map::getInstance();
	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();
	//vector< pair<int,SOCKET> > errors;

	int len;
	char buff[BUFFER_SIZE];
	
	unpack(message, buff, &len);

	WSABUF wsabuf;
	wsabuf.buf = buff;
	wsabuf.len = len;

	for (int i = 0; i < send_list.size(); i++)
	{
		SOCKET sock = send_list[i];
		PER_IO_DATA *ioInfo = new PER_IO_DATA;

		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		memcpy(ioInfo->buffer, buff, len);
		ioInfo->wsaBuf.len = len;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->RWmode = WRITE;

		int ret = WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

		if (ret == SOCKET_ERROR)
		{
			if (WSAGetLastError() == ERROR_IO_PENDING)
			{
				printf("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));

				// 큐에 들어감 ^.^
			}
			else
			{
				// 너에겐 수많은 이유가 있겠지... 하지만 아마도 그 수많은 이유들의 공통점은 소켓에 전송할 수 없는 것이 아닐까?
				free(ioInfo);
			}
			printf("Send Error (%d)\n", WSAGetLastError());
		}
		else
		{
			printf("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
		}
	}
}

void unpack(msg message, char *buf, int *size)
{
	int writebyte = 0;

	memcpy(buf + writebyte, &message.type, sizeof(int));
	writebyte += sizeof(int);
	memcpy(buf + writebyte, &message.len, sizeof(int));
	writebyte += sizeof(int);
	memcpy(buf + writebyte, message.buff, message.len);
	writebyte += message.len;

	*size = writebyte;
}

void closeClient(int id)
{
	Client_Map *CMap = Client_Map::getInstance();
	vector<SOCKET> send_list;

	SOCKET sock = CMap->find_id_to_sock(id);
	int ret = closesocket(sock);

	if (ret != WSAENOTSOCK)
	{
		// 처음으로 소켓을 닫을 때.
		int char_id = CMap->find_sock_to_id(sock);

		//아이디가 존재하지 않는 경우이므로, 이 경우는 제외한다.
		if (char_id == -1)
		{
			return;
		}

		set_multicast_in_room_except_me(char_id, send_list, false/*not autolock*/ );

		CMap->erase(sock);

		send_message(msg(ERASE_USER, sizeof(int), (char*)&char_id), send_list);		
	}
	else
	{
		//이미 삭제된 소켓.
	}
	
}