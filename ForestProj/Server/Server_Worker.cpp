#include <cstdio>
#include <vector>

#include "Completion_Port.h"
#include "types.h"
#include "Disc_user_map.h"
#include "Client_Map.h"
#include "msg.h"
#include "Sock_set.h"

using std::vector;

void set_single_cast(int , vector<SOCKET>& );
void set_multicast_in_room_except_me(int, vector<SOCKET>&);
void send_message(msg , vector<SOCKET> &);
void unpack(msg, char *, int *);

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

			for (int i = 0; i < 3; i++)
			{
				int *param[] = { &char_id, &x, &y };
				memcpy(buf + writebyte, param[i], sizeof(int));
				writebyte += sizeof(int);
			}

			set_single_cast(char_id, receiver);
			send_message(msg(type,len,buf), receiver);
			receiver.clear();

			// ���� ������ ĳ������ ������ �ٸ� ������ �����鿡�� �����Ѵ�.

			type = SET_USER;

			set_multicast_in_room_except_me(char_id, receiver);
			send_message(msg(type,len,buf), receiver);
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
			send_message(msg(type,len,buf), receiver);
			receiver.clear();	
		}
		else if( type == MOVE_USER )// ������ �̵��ϴ� ���
		{
			memcpy(buf, ioInfo->buffer + readbyte, len);

			int ntype, nlen;
			int cur_id, x_off, y_off;
			int cnt;
			char *pBuf;

			int writebyte;

			for (int i = 0; i < len; i += sizeof(int)* 3)
			{
				int *param[] = { &cur_id, &x_off, &y_off };
				for (int j = 0; j < 3; j++)
				{
					memcpy(param[j], buf+i, sizeof(int));
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
				send_message(msg(ntype,nlen,nbuf), receiver);
				receiver.clear();

				// ���� ���� �����鿡�� ���� ������� �˸�
				set_multicast_in_room_except_me(now->getID(), receiver);
				send_message(msg(ntype,sizeof(int),(char *)&cur_id), receiver);
				receiver.clear();

				// ĳ���͸� �ش� ��ǥ��ŭ �̵���Ŵ
				now->setX(now->getX() + x_off);
				now->setY(now->getY() + y_off);

				// ���ο� ���� �����鿡�� ���� �������� �˸�
				set_multicast_in_room_except_me(now->getID(), receiver);

				int id = now->getID(), x = now->getX(), y = now->getY();
				pBuf = nbuf;
				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &id, &x, &y };
					memcpy(pBuf, param[i], sizeof(int));
					pBuf += sizeof(int);
				}

				

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