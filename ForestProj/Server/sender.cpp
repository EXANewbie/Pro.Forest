#include <WinSock2.h>

#include <mutex>
#include <set>
#include <map>

#include "que.h"
#include "msg.h"
#include "cmap.h"
#include "types.h"

using namespace std;


void send_message(msg, SYNCHED_CHARACTER_MAP *, vector<SOCKET> &);

void set_single_cast(SOCKET sock, vector<SOCKET>& send_list);
void set_broad_cast_except_me(SYNCHED_CHARACTER_MAP *, SOCKET, vector<SOCKET>& );
void set_broad_cast_all(SYNCHED_CHARACTER_MAP *, vector<SOCKET>&);
//void set_multicast_exist_chars(SYNCHED_CHARACTER_MAP *, vector<SOCKET>& );
void send_erase_user_message(SYNCHED_CHARACTER_MAP *chars, vector< pair<int, SOCKET> >& errors);

void sender(set<SOCKET> *sock_set, SYNCHED_QUEUE *que, SYNCHED_CHARACTER_MAP *chars) {

	while (true)
	{
		while (!que->empty()){
			msg tmp_msg = que->front();
			que->pop();

			int type;
			int len;
			SOCKET sock;
			int char_id, x, y;

			char buff[1024];
			char *pBuf;
			vector<SOCKET> receiver;

			switch (tmp_msg.type)
			{
			case CONNECT:

				//정보 받고
				memcpy(&sock, tmp_msg.buff, sizeof(int));

				if (strcmp("HELLO SERVER!", tmp_msg.buff + sizeof(int))){
					printf("Invalid Client.");
				}

				{
//					static int id = 1;
					int id = (int)sock;
					Character c(id);
					char_id = id;
					x = c.getX();
					y = c.getY();
//					id++;


					chars->lock();
					sock_set->erase(sock);
					chars->insert(sock, c);
					chars->unlock();
				} // 임시 캐릭터 객체를 생성 후, x와 y의 초기값을 가져온다.

				set_single_cast(sock, receiver);

				type = INIT;
				len = 3 * sizeof(int);

				pBuf = buff;

				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &char_id, &x, &y };
					memcpy(pBuf, param[i], sizeof(int));
					pBuf += sizeof(int);
				}

				send_message(msg(type, len, buff), chars, receiver);
				receiver.clear();

				// CONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.

				set_single_cast(sock, receiver);

				type = SET_USER;
				len = 3 * sizeof(int);

				chars->lock();
				for (auto itr = chars->begin(); itr != chars->end(); itr++)
				{
					const int tID = itr->second.getID();
					const int tx = itr->second.getX();
					const int ty = itr->second.getY();

					if (tID == char_id) // 캐릭터 맵에 현재 들어간 내 객체의 정보를 보내려 할 때는 건너뛴다.
						continue;

					pBuf = buff;
					for (int i = 0; i < 3; i++)
					{
						const int *param[] = { &tID, &tx, &ty };
						memcpy(pBuf, param[i], sizeof(int));
						pBuf += sizeof(int);
					}

					send_message(msg(type, len, buff), chars, receiver);
				}
				chars->unlock();
				receiver.clear();

				// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.

				set_broad_cast_except_me(chars, sock, receiver);

				type = SET_USER;
				len = 3 * sizeof(int);

				pBuf = buff;
				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &char_id, &x, &y };
					memcpy(pBuf, param[i], sizeof(int));
					pBuf += sizeof(int);
				}

				send_message(msg(type, len, buff), chars, receiver);
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
							  len = tmp_msg.len - sizeof(SOCKET);
							  int cur_id, x_off, y_off;

							  memcpy(&sock, pBuf, sizeof(SOCKET));
							  pBuf += sizeof(SOCKET);

							  for (int i = 0; i < len; i += sizeof(int)* 3)
							  {
								  int *param[] = { &cur_id, &x_off, &y_off };
								  for (int j = 0; j < 3; j++)
								  {
									  memcpy(param[j], pBuf, sizeof(int));
									  pBuf += sizeof(int);
								  }

								  Character *now = chars->find(cur_id);
								  now->setX(now->getX() + x_off);
								  now->setY(now->getY() + y_off);
								  printf("id : %d, x_off : %d, y_off : %d\n", cur_id, x_off, y_off);
							  }

							  pBuf = tmp_msg.buff + sizeof(SOCKET);

							  set_broad_cast_all(chars, receiver);
							  send_message(msg(tmp_msg.type,len,pBuf), chars, receiver);
							  receiver.clear();
			}
				break;
			case DISCONN:
			{
				memcpy(&sock, tmp_msg.buff, sizeof(SOCKET));
				char_id = chars->find(sock)->getID();
				printf("Character %d(Socket %d) request disconnecting\n", char_id, sock);
				vector< pair<int, SOCKET> > errors;
				errors.push_back(make_pair(char_id, sock));
				send_erase_user_message(chars, errors);
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

void set_single_cast(SOCKET sock, vector<SOCKET>& send_list)
{
	send_list.push_back(sock);
}

void set_broad_cast_except_me(SYNCHED_CHARACTER_MAP *chars, SOCKET sock, vector<SOCKET>& send_list)
{
	chars->lock();
	for (auto iter = chars->begin(); iter != chars->end(); iter++) {
		if (iter->first == sock)
			continue;

		send_list.push_back(iter->first);
	}
	chars->unlock();
}
/*
void set_multicast_exist_chars(SYNCHED_SOCKET_MAP *socks, SYNCHED_CHARACTER_MAP *chars, vector<SOCKET>& send_list)
{
	vector<int> id_box;
	chars->lock();
	for (auto itr = chars->begin(); itr != chars->end(); itr++)
	{
		id_box.push_back(itr->first);
	}
	chars->unlock();

	socks->lock();
	for (int i = 0; i < id_box.size(); i++)
	{
		send_list.push_back(socks->find(id_box[i]));
	}
	socks->unlock();
}
*/
void set_broad_cast_all(SYNCHED_CHARACTER_MAP *chars, vector< SOCKET >& send_list)
{
	chars->lock();
	for (auto iter = chars->begin(); iter != chars->end(); iter++) {
		send_list.push_back(iter->first);
	}
	chars->unlock();
}

void send_message(msg message, SYNCHED_CHARACTER_MAP *chars, vector<SOCKET> &send_list) {
	vector< pair<int,SOCKET> > errors;
	for (int i = 0; i < send_list.size(); i++)
	{
		int ret = send(send_list[i], (char *)&message.type, sizeof(int), 0);
		if (ret != sizeof(int))
		{
			Character c = *chars->find(send_list[i]);
			errors.push_back( make_pair(c.getID(),send_list[i]) );
			continue;
		}
		send(send_list[i], (char *)&message.len, sizeof(int), 0);
		send(send_list[i], (char *)&message.buff, message.len, 0);
	}

	// 전송 실패한 유저들을 모아서 전송

	if (!errors.empty())
	{
		send_erase_user_message(chars, errors);
	}
}