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
void set_multicast_in_room_except_me(int, vector<SOCKET>&);
void send_message(msg , vector<SOCKET> &);
void unpack(msg, char *, int *);
void closeClient(SOCKET sock);

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
				CMap->lock();
				int char_id = CMap->find_sock_to_id(sock);
				CMap->unlock();
				printf("sock : %d char_id : %d", sock, char_id);

				closeClient(sock);
				free(handleInfo); free(ioInfo);
				continue;
			}

			//�޽����� ���� �� ó���ؾ��� ��� �κ�

			int readbyte = 0;
			int type, len;
			char buf[BUFFER_SIZE];

			memcpy(&type, ioInfo->buffer + readbyte, sizeof(int));
			readbyte += sizeof(int);
			memcpy(&len, ioInfo->buffer + readbyte, sizeof(int));
			readbyte += sizeof(int);

			if (type == DISCONN) // �������� ������ ���
			{
				memcpy(buf, ioInfo->buffer + readbyte, len);
				if (strcmp("BYE SERVER!", buf))
				{
					//��¥ Ŭ���̾�Ʈ
				}

				printf("Nomal turn off ");
				CMap->lock();
				int char_id = CMap->find_sock_to_id(sock);
				CMap->unlock();
				printf("sock : %d char_id : %d", sock, char_id);

				closeClient(sock);
				free(handleInfo); free(ioInfo);
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
				static int id = 1;
				int writebyte;

				// ĳ���� ��ü�� ���� ��
				Character c(id);
				char_id = id;
				x = c.getX();
				y = c.getY();
				id++;

				sock_set->erase(sock);
				CMap->lock();
				CMap->insert(char_id, sock, c);
				CMap->unlock();

				// x�� y�� �ʱⰪ�� �����´�.	
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

				// ���� ������ ĳ������ ������ �ٸ� ������ �����鿡�� �����Ѵ�.

				type = SET_USER;

				set_multicast_in_room_except_me(char_id, receiver);
				send_message(msg(type, len, buf), receiver);
				receiver.clear();

				// CONNECT�� ������ �������� �ٸ� ��ü���� ������ �����Ѵ�.

				type = SET_USER;
				len = 0;

				writebyte = 0;

				CMap->lock();
				for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
				{
					const int tID = itr->second.getID();
					const int tx = itr->second.getX();
					const int ty = itr->second.getY();

					if (tID == char_id) // ĳ���� �ʿ� ���� �� �� ��ü�� ������ ������ �� ���� �ǳʶڴ�.
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

					ntype = ERASE_USER;
					nlen = sizeof(int)*cnt;

					set_single_cast(now->getID(), receiver);
					send_message(msg(ntype, nlen, nbuf), receiver);
					receiver.clear();

					// ���� ���� �����鿡�� ���� ������� �˸�
					set_multicast_in_room_except_me(now->getID(), receiver);
					send_message(msg(ntype, sizeof(int), (char *)&cur_id), receiver);
					receiver.clear();

					// ĳ���͸� �ش� ��ǥ��ŭ �̵���Ŵ
					now->setX(now->getX() + x_off);
					now->setY(now->getY() + y_off);

					// ���ο� ���� �����鿡�� ���� �������� �˸�
					int id = now->getID(), x = now->getX(), y = now->getY();
					pBuf = nbuf;
					for (int i = 0; i < 3; i++)
					{
						int *param[] = { &id, &x, &y };
						memcpy(pBuf, param[i], sizeof(int));
						pBuf += sizeof(int);
					}

					set_multicast_in_room_except_me(now->getID(), receiver);
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
					// ���� ���� �߻�
				}
			}
		}
		else // WRITE
		{
			if (bytesTrans == 0) // �ùٸ��� ���� ������ ���
			{
				printf("�� ��µǴ°� ����?��\n");
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
				// ť�� �� ^.^
			}
			else
			{
				// �ʿ��� ������ ������ �ְ���... ������ �Ƹ��� �� ������ �������� �������� ���Ͽ� ������ �� ���� ���� �ƴұ�?
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

void closeClient(SOCKET sock)
{
	Client_Map *CMap = Client_Map::getInstance();
	vector<SOCKET> send_list;

	EnterCriticalSection(&cs);
	int ret = closesocket(sock);
	LeaveCriticalSection(&cs);

	if (ret != WSAENOTSOCK)
	{
		// ó������ ������ ���� ��.
		CMap->lock();
		int char_id = CMap->find_sock_to_id(sock);
		CMap->unlock();

		//���̵� �������� �ʴ� ����̹Ƿ�, �� ���� �����Ѵ�.
		if (char_id == -1)
		{
			//���̵� �������� �ʴ� ����̹Ƿ�, �� ���� �����Ѵ�.
			return;
		}

		set_multicast_in_room_except_me(char_id, send_list);

		CMap->lock();
		CMap->erase(sock);
		CMap->unlock();

		send_message(msg(ERASE_USER, sizeof(int), (char*)&char_id), send_list);		
	}
	else
	{
		//�̹� ������ ����.
	}
	
}