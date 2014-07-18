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
			if (bytesTrans == 0) // �ùٸ��� ���� ������ ���
			{
				printf("@Abnomal turn off ");
				remove_valid_client(sock, handleInfo, ioInfo);
				continue;
			}

			//�޽����� ���� �� ó���ؾ��� ��� �κ�

			int readbyte = 0;
			int type, len;
			char buf[BUFFER_SIZE];
			{
				int *param[] = { &type, &len };
				copy_to_param(param, 2, ioInfo->buffer + readbyte);
				readbyte += 2 * sizeof(int);
			}
			
			if (type == DISCONN) // �������� ������ ���
			{
				memcpy(buf, ioInfo->buffer + readbyte, len);
				if (strcmp("BYE SERVER!", buf))
				{
					//��¥ Ŭ���̾�Ʈ
				}

				printf("Nomal turn off ");
				remove_valid_client(sock, handleInfo, ioInfo);
				continue;
			}
			else if (type == CONNECT) // ���� ���� ���
			{
				memcpy(buf, ioInfo->buffer + readbyte, len);
				if (strcmp("HELLO SERVER!", buf))
				{
					//��¥ Ŭ���̾�Ʈ
				}

				int char_id;
				int x, y;
				static int id = 0;
				int writebyte;

				// ĳ���� ��ü�� ���� ��
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

				// x�� y�� �ʱⰪ�� �����´�.	
				len = 3 * sizeof(int);
				{
					int *param[] = { &char_id, &x, &y };
					copy_to_buffer(buf, param, 3);
				}
				
				set_single_cast(char_id, receiver);
				send_message(msg(INIT, len, buf), receiver);
				receiver.clear();

				// ���� ������ ĳ������ ������ �ٸ� ������ �����鿡�� �����Ѵ�.

				set_multicast_in_room_except_me(char_id, receiver, true/*autolock*/ );
				send_message(msg(SET_USER, len, buf), receiver);
				receiver.clear();

				// CONNECT�� ������ �������� �ٸ� ��ü���� ������ �����Ѵ�.
				len = 0;
				writebyte = 0;
				char *pBuf = buf;
				CMap->lock();
				for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
				{
					int tID = itr->second.getID();
					int tx = itr->second.getX();
					int ty = itr->second.getY();

					if (tID == char_id) // ĳ���� �ʿ� ���� �� �� ��ü�� ������ ������ �� ���� �ǳʶڴ�.
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
				send_message(msg(SET_USER, len, buf), receiver);
				receiver.clear();
			}
			else if (type == MOVE_USER)// ������ �̵��ϴ� ���
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

					// ������ ���� �������� ������ ������
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
					send_message(msg(ERASE_USER, nlen, nbuf), receiver);
					receiver.clear();

					// ���� ���� �����鿡�� ���� ������� �˸�
					set_multicast_in_room_except_me(now->getID(), receiver, true/*autolock*/ );
					send_message(msg(ERASE_USER, sizeof(int), (char *)&cur_id), receiver);
					receiver.clear();

					// ĳ���͸� �ش� ��ǥ��ŭ �̵���Ŵ
					now->setX(now->getX() + x_off);
					now->setY(now->getY() + y_off);

					// ���ο� ���� �����鿡�� ���� �������� �˸�
					int id = now->getID(), x = now->getX(), y = now->getY();
					int *param2[] = { &id, &x, &y };
					copy_to_buffer(nbuf, param2, 3);

					set_multicast_in_room_except_me(now->getID(), receiver, true/*autolock*/ );
					send_message(msg(SET_USER, 3 * sizeof(int), nbuf), receiver);
					receiver.clear();

					// ���ο� ���� �������� ������ �ҷ���
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
					// ���� ���� �߻�
				}
			}
		}
		else // WRITE
		{
			if (bytesTrans == 0) // �ùٸ��� ���� ������ ���
			{
				printf("�� ��µǴ°� ����?��\n");
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