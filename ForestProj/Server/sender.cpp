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

void sender(SYNCHED_QUEUE *que, SYNCHED_SOCKET_MAP *socks, SYNCHED_CHARACTER_MAP *chars) {

	while (true)
	{
		// 硫붿떆吏媛 ??寃쎌슦 ??臾몄옣?ㅼ씠 ?ㅽ뻾
		if (!que->empty()){
			msg tmp_msg = que->front();
			que->pop();

			int type;
			int len;
			int id;
			int x = 0, y = 0;

			char buff[1024];
			vector<SOCKET> receiver;

			switch (tmp_msg.type)
			{
			case CONNECT:
				//?뺣낫 諛쏄퀬
				for (int i = 0; i < sizeof(int); i++)
					((char *)&id)[i] = tmp_msg.buff[i];

				if (strcmp("HELLO SERVER!", tmp_msg.buff + sizeof(int))){
					printf("Invalid Client.");
				}

				//?뺣낫 媛怨?
				chars->insert(id, Character(id));

				{
					Character c = chars->find(id);
					x = c.getX();
					y = c.getY();
				}
				for (int i = 0; i < sizeof(int); i++)
				{
					buff[i] = ((char *)&id)[i];
					buff[i + sizeof(int)] = ((char *)&x)[i];
					buff[i + 2 * sizeof(int)] = ((char *)&y)[i];
				}
				len = 3 * sizeof(int);
				type = INIT;

				//?뺣낫 蹂대궡湲?

				set_single_cast(socks, id, receiver);
				send_message(msg(type, len, buff), receiver);

				receiver.clear();

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

void set_broad_cast_all(SYNCHED_SOCKET_MAP *socks, vector<SOCKET>& send_list)
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