#include <WinSock2.h>


#include <mutex>
#include <map>

#include "que.h"
#include "msg.h"

using namespace std;

void sender(SYNCHED_QUEUE *que,map<int, SOCKET> *socks) {
	while (true)
	{
		// 메시지가 올 경우 이 문장들이 실행
		if (!que->empty()){
			msg tmp_msg = que->front();
			que->pop();

//			sock_mtx.lock();
			vector<int> vec;
			for (auto iter = socks->begin(); iter != socks->end(); iter++) {

				int ret = send(iter->second, (char *)&tmp_msg.len, sizeof(int), 0);

				if (ret != sizeof(int)) {
					(*socks)[iter->first] = SOCKET_ERROR;
					continue;
				}

				if (iter->second == SOCKET_ERROR)
					continue;

				send(iter->second, tmp_msg.buff, tmp_msg.len, 0);
			}
			
//			sock_mtx.unlock();
		}

	}
}