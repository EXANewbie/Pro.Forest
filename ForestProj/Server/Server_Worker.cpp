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
		if ( bytesTrans == 0) // �ùٸ��� ���� ������ ���
		{
			closesocket(sock);
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
			if (strcmp(DSCMSG, buf))
			{
				//��¥ Ŭ���̾�Ʈ
			}

			CMap->lock();
			CMap->erase(sock);
			CMap->unlock();
						
			closesocket(sock); // ���� ������ �ݴ´�.

		}
		else if (type == CONNECT) // ���� ���� ���
		{
			memcpy(buf, ioInfo->buffer + readbyte, len);
			if (strcmp(CONMSG, buf))
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

			for (int i = 0; i < 5; i++)
			{
				int *param[] = { &type, &len, &char_id, &x, &y };
				memcpy(buf + writebyte, param[i], sizeof(int));
				writebyte += sizeof(int);
			}

			set_single_cast(char_id, receiver);
			send_message(buf, writebyte, receiver);
			receiver.clear();

			// ���� ������ ĳ������ ������ �ٸ� ������ �����鿡�� �����Ѵ�.

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

			// CONNECT�� ������ �������� �ٸ� ��ü���� ������ �����Ѵ�.

			type = SET_USER;
			len = 0;

			writebyte = sizeof(int) * 2;

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
		else // ������ ������ ���
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
		/*SEND�� ó���ؾߵ� �κ�
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = bytesTrans;
		WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, NULL, NULL);
		*/ 

		// �ٽ� ���� �غ� �ϴ� �κ�
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

	// ���� ������ �������� ��Ƽ� ����

	//if (!errors.empty())
	//{
	//send_erase_user_message(CMap, errors);
	//}
}