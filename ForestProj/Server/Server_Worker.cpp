#include <cstdio>
#include <string>
#include <vector>

#include "../protobuf/connect.pb.h"
#include "../protobuf/disconn.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"

#include "Completion_Port.h"
#include "types.h"
#include "Disc_user_map.h"
#include "Client_Map.h"
#include "msg.h"
#include "Sock_set.h"

#include <Windows.h>

using std::vector;
using std::string;

void set_single_cast(int , vector<SOCKET>& );
void set_multicast_in_room_except_me(int, vector<SOCKET>&,bool);
void send_message(msg , vector<SOCKET> &);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(SOCKET, LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int );
void copy_to_param(int **, int, char *);

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
				printf("@Abnomal turn off ");
				remove_valid_client(sock, handleInfo, ioInfo);
				continue;
			}

			//메시지를 받은 후 처리해야할 계산 부분

			int readbyte = 0;
			int type, len;
			char buf[BUFFER_SIZE];
			{
				int *param[] = { &type, &len };
				copy_to_param(param, 2, ioInfo->buffer + readbyte);
				readbyte += 2 * sizeof(int);
			}
			memcpy(buf, ioInfo->buffer + readbyte, len); // 버퍼 내의 메시지를 일단 받는다!!

			if (type == PDISCONN) // 정상적인 종료의 경우
			{
				DISCONN::CONTENTS disconn;

				disconn.ParseFromString(buf);
				if (disconn.data() == "BYE SERVER!")
				{
					//가짜 클라이언트
				}

				printf("Nomal turn off\n");
				remove_valid_client(sock, handleInfo, ioInfo);
				continue;
			}
			else if (type == PCONNECT) // 새로 들어온 경우
			{
				CONNECT::CONTENTS connect;

				connect.ParseFromString(buf);
				if (connect.data() == "HELLO SERVER!")
				{
					//가짜 클라이언트
				}

				int char_id;
				int x, y;
				static int id = 0;
				int writebyte;

				// 캐릭터 객체를 생성 후
				int copy_id = InterlockedIncrement((unsigned *)&id);
				Character c(copy_id);
				char_id = copy_id;
				x = c.getX();
				y = c.getY();

				bool isValid = false;
				while(true)
				{
					CMap->lock();
					isValid = CMap->insert(char_id, sock, c);
					CMap->unlock();
					if (isValid == true)
					{
						sock_set->erase(sock);
						break;
					}
					else
					{
						SleepEx(100, true);
					}
				}

				// x와 y의 초기값을 가져온다.	
				len = 3 * sizeof(int);

				INIT::CONTENTS init;
				auto myData = init.data();
				{
					int *param[] = { &char_id, &x, &y };
					copy_to_buffer(buf, param, 3);
				}
				
				set_single_cast(char_id, receiver);
				send_message(msg(PINIT, len, buf), receiver);
				receiver.clear();

				// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.

				set_multicast_in_room_except_me(char_id, receiver, true/*autolock*/ );
				send_message(msg(PSET_USER, len, buf), receiver);
				receiver.clear();

				// PCONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.
				len = 0;
				writebyte = 0;
				char *pBuf = buf;
				CMap->lock();
				for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
				{
					int tID = itr->second.getID();
					int tx = itr->second.getX();
					int ty = itr->second.getY();

					if (tID == char_id) // 캐릭터 맵에 현재 들어간 내 객체의 정보를 보내려 할 때는 건너뛴다.
						continue;

					if (tx == x && ty == y)
					{
						len += 3 * sizeof(int);
						int *param[] = { &tID, &tx, &ty };
						copy_to_buffer(pBuf, param, 3);
						pBuf += 3 * sizeof(int);
					}
				}
				CMap->unlock();

				set_single_cast(char_id, receiver);
				send_message(msg(PSET_USER, len, buf), receiver);
				receiver.clear();
			}
			else if (type == PMOVE_USER)// 유저가 이동하는 경우
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
					copy_to_param(param, 3, buf);

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
					nlen = sizeof(int)*cnt;

					set_single_cast(now->getID(), receiver);
					send_message(msg(PERASE_USER, nlen, nbuf), receiver);
					receiver.clear();

					// 기존 방의 유저들에게 내가 사라짐을 알림
					set_multicast_in_room_except_me(now->getID(), receiver, true/*autolock*/ );
					send_message(msg(PERASE_USER, sizeof(int), (char *)&cur_id), receiver);
					receiver.clear();

					// 캐릭터를 해당 좌표만큼 이동시킴
					now->setX(now->getX() + x_off);
					now->setY(now->getY() + y_off);

					// 새로운 방의 유저들에게 내가 등장함을 알림
					int id = now->getID(), x = now->getX(), y = now->getY();
					int *param2[] = { &id, &x, &y };
					copy_to_buffer(nbuf, param2, 3);

					set_multicast_in_room_except_me(now->getID(), receiver, true/*autolock*/ );
					send_message(msg(PSET_USER, 3 * sizeof(int), nbuf), receiver);
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
							int *param3[] = { &nid, &nx, &ny };
							copy_to_buffer(pBuf, param3, 3);
							pBuf += 3 * sizeof(int);
							cnt++;
						}
					}
					CMap->unlock();
					set_single_cast(now->getID(), receiver);
					send_message(msg(PSET_USER, 3 * sizeof(int)* cnt, nbuf), receiver);
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
				printf("나 출력되는거 맞음?ㅋ\n");
				remove_valid_client(sock, handleInfo, ioInfo);
				continue;
			}

			puts("MESSAGE SEND!");
			free(ioInfo);
			printf("k Decrement %d\n", InterlockedDecrement((unsigned int *)&k));
		}
	}

	return 0;
}

