#include <WinSock2.h>

#include <mutex>
#include <map>

#include "que.h"
#include "msg.h"
#include "smap.h"
#include "cmap.h"
#include "types.h"

using namespace std;

void send_message(msg, vector<SOCKET>&);

void set_single_cast(SYNCHED_SOCKET_MAP *, int, vector<SOCKET>&);
void set_broad_cast_except_me(SYNCHED_SOCKET_MAP *, int, vector<SOCKET>&);
void set_broad_cast_all(SYNCHED_SOCKET_MAP *, vector<SOCKET>&);
void set_multicast_exist_chars(SYNCHED_SOCKET_MAP *, SYNCHED_CHARACTER_MAP *, vector<SOCKET>& );

void sender(SYNCHED_QUEUE *que, SYNCHED_SOCKET_MAP *socks, SYNCHED_CHARACTER_MAP *chars) {

	while (true)
	{
		// 메시지가 올 경우 이 문장들이 실행
		if (!que->empty()){
			msg tmp_msg = que->front();
			que->pop();

			int type;
			int len;
			int id;
			int x, y;

			char buff[1024];
			char *pBuf;
			vector<SOCKET> receiver;

			switch (tmp_msg.type)
			{
			case CONNECT:
				//정보 받고
				for (int i = 0; i < sizeof(int); i++)
					((char *)&id)[i] = tmp_msg.buff[i];

				if (strcmp("HELLO SERVER!", tmp_msg.buff + sizeof(int))){
					printf("Invalid Client.");
				}

				{
					Character c(id);
					x = c.getX();
					y = c.getY();

					chars->lock();
					chars->insert(id, Character(id));
					chars->unlock();
				} // 임시 캐릭터 객체를 생성 후, x와 y의 초기값을 가져온다.

				set_single_cast(socks, id, receiver);

				type = INIT;
				len = 3 * sizeof(int);

				pBuf = buff;
				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &id, &x, &y };
					memcpy(pBuf, param[i], sizeof(int));
					pBuf += sizeof(int);
				}

				send_message(msg(type, len, buff), receiver);
				receiver.clear();

				// CONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.

				set_single_cast(socks, id, receiver);

				type = SET_USER;
				len = 3 * sizeof(int);

				chars->lock();
				for (auto itr = chars->begin(); itr != chars->end(); itr++)
				{
					const int tID = itr->second.getID();
					const int tx = itr->second.getX();
					const int ty = itr->second.getY();

					pBuf = buff;
					for (int i = 0; i < 3; i++)
					{
						const int *param[] = { &tID, &tx, &ty };
						memcpy(pBuf, param[i], sizeof(int));
						pBuf += sizeof(int);
					}

					send_message(msg(type, len, buff), receiver);
				}
				chars->unlock();
				receiver.clear();

				// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.

				set_multicast_exist_chars(socks, chars, receiver);

				type = SET_USER;
				len = 3 * sizeof(int);

				pBuf = buff;
				for (int i = 0; i < 3; i++)
				{
					int *param[] = { &id, &x, &y };
					memcpy(pBuf, param[i], sizeof(int));
					pBuf += sizeof(int);
				}

				send_message(msg(type, len, buff), receiver);
				
				// 모든 처리가 끝난 후 캐릭터 맵에 삽입

				break;
			case INIT:
				break;
			case SET_USER:
				break;
			case MOVE_USER:
				break;
			case DISCONN:
				break;
			case ERASE_USER:
				break;
			default:
				break;
			}

		}

	}
}

void set_single_cast(SYNCHED_SOCKET_MAP *socks, int id, vector<SOCKET>& send_list)
{
	socks->lock();
	send_list.push_back(socks->find(id));
	socks->unlock();
}

void set_broad_cast_except_me(SYNCHED_SOCKET_MAP *socks, int id, vector<SOCKET>& send_list)
{
	socks->lock();
	for (auto iter = socks->begin(); iter != socks->end(); iter++) {
		if (iter->first == id)
			continue;

		send_list.push_back(iter->second);
	}
	socks->unlock();
}

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

void set_broad_cast_all(SYNCHED_SOCKET_MAP *socks, vector< SOCKET >& send_list)
{
	socks->lock();
	for (auto iter = socks->begin(); iter != socks->end(); iter++) {
		send_list.push_back(iter->second);
	}
	socks->unlock();
}

void send_message(msg msg, vector<SOCKET> &send_list) {
	for (int i = 0; i < send_list.size(); i++) {
		int ret = send(send_list[i], (char *)&msg.type, sizeof(int), 0);
		if (ret != sizeof(int)) {
			//error
			continue;
		}
		send(send_list[i], (char *)&msg.len, sizeof(int), 0);
		send(send_list[i], (char *)&msg.buff, msg.len, 0);
	}
}