#include <WinSock2.h>

#include <mutex>
#include <set>
#include <map>

#include "que.h"
#include "msg.h"
//#include "CMap->h"
#include "types.h"
#include "character.h"
#include "Client_Map.h"
#include "Disc_user_map.h"

using namespace std;


void send_message(msg, vector<SOCKET> &);

void set_single_cast(int, vector<SOCKET>&);
void set_broad_cast_except_me(int, vector<SOCKET>&);
void set_broad_cast_all(vector<SOCKET>&);
void set_multicast_in_room_except_me(int, vector<SOCKET>&);
//void send_erase_user_message(Client_Map *, vector< pair<int, SOCKET> >& );

void sender(set<SOCKET> *sock_set)
{
	Client_Map *CMap = Client_Map::getInstance();
	SYNCHED_QUEUE *que = SYNCHED_QUEUE::getInstance();
	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();
	while (true)
	{
		while (!que->empty()){
			msg tmp_msg = que->front();
			que->pop();

			int type;
			int len;
			//SOCKET sock;
			int char_id, x, y;

			char buff[1024];
			char *pBuf;
			vector<SOCKET> receiver;

			switch (tmp_msg.type)
			{
			case CONNECT:

				//정보 받고

				if (strcmp("HELLO SERVER!", tmp_msg.buff + sizeof(int))){
					printf("Invalid Client.");
				}

				{
					SOCKET sock;
					memcpy(&sock, tmp_msg.buff, sizeof(int));

					static int id = 1;
					//int id = (int)sock;
					Character c(id);
					char_id = id;
					x = c.getX();
					y = c.getY();
					id++;

					//chars->lock();
					sock_set->erase(sock);
					CMap->lock();
					CMap->insert(char_id, sock, c);
					CMap->unlock();
					//chars->unlock();
				} // 임시 캐릭터 객체를 생성 후, x와 y의 초기값을 가져온다.

				set_single_cast(char_id, receiver);

				type = INIT;
				len = 3 * sizeof(int);

				pBuf = buff;

				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &char_id, &x, &y };
					memcpy(pBuf, param[i], sizeof(int));
					pBuf += sizeof(int);
				}

				send_message(msg(type, len, buff), receiver);
				receiver.clear();

				// CONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.

				set_single_cast(char_id, receiver);

				type = SET_USER;
				len = 0;
				pBuf = buff;
				//chars->lock();
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
							memcpy(pBuf, param[i], sizeof(int));
							pBuf += sizeof(int);
						}
						len += 3 * sizeof(int);
					}
				}
				CMap->unlock();
				//chars->unlock();
				send_message(msg(type, len, buff), receiver);
				receiver.clear();

				// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.

				set_multicast_in_room_except_me(char_id, receiver);

				type = SET_USER;
				len = 3 * sizeof(int);

				pBuf = buff;
				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &char_id, &x, &y };
					memcpy(pBuf, param[i], sizeof(int));
					pBuf += sizeof(int);
				}

				send_message(msg(type, len, buff), receiver);
				receiver.clear();

				// 모든 처리가 끝난 후 캐릭터 맵에 삽입

				break;
			case INIT:
				break;
			case SET_USER:
				break;
			case MOVE_USER:
			{
							  char *pBuf = tmp_msg.buff;
							  len = tmp_msg.len;
							  int cur_id, x_off, y_off;
							  int cnt;

							  for (int i = 0; i < len; i += sizeof(int)* 3)
							  {
								  int *param[] = { &cur_id, &x_off, &y_off };
								  for (int j = 0; j < 3; j++)
								  {
									  memcpy(param[j], pBuf, sizeof(int));
									  pBuf += sizeof(int);
								  }
								  CMap->lock();
								  Character *now = CMap->find_id_to_char(cur_id);
								  CMap->unlock();

								  // 기존의 방의 유저들의 정보를 삭제함
								  pBuf = buff;
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
								  set_single_cast(now->getID(), receiver);
								  send_message(msg(ERASE_USER, sizeof(int)* cnt, buff), receiver);
								  receiver.clear();

								  // 기존 방의 유저들에게 내가 사라짐을 알림
								  set_multicast_in_room_except_me(now->getID(), receiver);
								  send_message(msg(ERASE_USER, sizeof(int), (char *)&cur_id), receiver);
								  receiver.clear();

								  now->setX(now->getX() + x_off);
								  now->setY(now->getY() + y_off);

								  // 새로운 방의 유저들에게 내가 등장함을 알림
								  set_multicast_in_room_except_me(now->getID(), receiver);

								  int id = now->getID(), x = now->getX(), y = now->getY();
								  char *pBuf = buff;
								  for (int i = 0; i < 3; i++)
								  {
									  int *param[] = { &id, &x, &y };
									  memcpy(pBuf, param[i], sizeof(int));
									  pBuf += sizeof(int);
								  }

								  send_message(msg(SET_USER, 3 * sizeof(int), buff), receiver);
								  receiver.clear();

								  // 새로운 방의 유저들의 정보를 불러옴
								  pBuf = buff;
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
								  send_message(msg(SET_USER, 3 * sizeof(int)* cnt, buff), receiver);
								  receiver.clear();

								  printf("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);
							  }
			}
				break;
			case DISCONN:
			{
							char *pBuf = tmp_msg.buff;
							SOCKET sock;
							memcpy(&sock, pBuf, sizeof(SOCKET));
							pBuf += sizeof(SOCKET);

							auto pairs = Disc_User->find(sock);
							if (pairs != Disc_User->end()) // 최초 처리일 경우
							{
								char_id = pairs->second;
								Disc_User->erase(pairs->first);

								memcpy(buff, &char_id, sizeof(int));
								msg erase_msg(ERASE_USER, 4, buff);

								set_multicast_in_room_except_me(char_id, receiver);
								send_message(erase_msg, receiver);
								receiver.clear();
								CMap->lock();
								CMap->erase(char_id);
								CMap->unlock();
							}
							else {
								// do nothing
							}
							//char_id = CMap->find(sock)->getID();
							//printf("Character %d(Socket %d) request disconnecting\n", char_id, sock);
							//vector< pair<int, SOCKET> > errors;
							//errors.push_back(make_pair(char_id, sock));
							//send_erase_user_message(chars, errors);
			}
				break;
			case ERASE_USER:
				break;
			default:
				break;
			}
		}
		Sleep(1);
	}
}

void set_single_cast(int id, vector<SOCKET>& send_list)
{
	Client_Map *CMap = Client_Map::getInstance();
	CMap->lock();
	SOCKET sock = CMap->find_id_to_sock(id);
	CMap->unlock();
	send_list.push_back(sock);
}

void set_broad_cast_except_me(int id, vector<SOCKET>& send_list)
{
	Client_Map *CMap = Client_Map::getInstance();
	CMap->lock();
	for (auto iter = CMap->begin(); iter != CMap->end(); iter++) {
		if (iter->first == id)
			continue;
		SOCKET sock = CMap->find_id_to_sock(iter->first);
		send_list.push_back(sock);
	}
	CMap->unlock();
}

void set_multicast_in_room_except_me(int id, vector<SOCKET>& send_list)
{
	Client_Map *CMap = Client_Map::getInstance();

	CMap->lock();

	auto now = CMap->find_id_to_char(id);
	printf("cc : %d %d\n", now->getX(), now->getY());

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

void set_broad_cast_all(vector< SOCKET >& send_list)
{
	Client_Map *CMap = Client_Map::getInstance();
	CMap->lock();
	for (auto iter = CMap->begin(); iter != CMap->end(); iter++) {
		SOCKET sock = CMap->find_id_to_sock(iter->first);
		send_list.push_back(sock);
	}
	CMap->unlock();
}

void send_message(msg message, vector<SOCKET> &send_list) {
	Client_Map *CMap = Client_Map::getInstance();
	SYNCHED_QUEUE *que = SYNCHED_QUEUE::getInstance();
	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();
	//vector< pair<int,SOCKET> > errors;
	for (int i = 0; i < send_list.size(); i++)
	{
		SOCKET sock = send_list[i];
		int ret = send(sock, (char *)&message.type, sizeof(int), 0);
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
		send(sock, (char *)&message.buff, message.len, 0);
	}

	// 전송 실패한 유저들을 모아서 전송

	//if (!errors.empty())
	//{
	//send_erase_user_message(CMap, errors);
	//}
}