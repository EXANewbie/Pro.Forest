#include <WinSock2.h>

#include <mutex>
#include <map>

#include "que.h"
#include "msg.h"
#include "smap.h"


using namespace std;

void sender(SYNCHED_QUEUE *que, SYNCHED_MAP *socks) {
	
	while (true)
	{
		// 메시지가 올 경우 이 문장들이 실행
		if (!que->empty()){
			msg tmp_msg = que->front();
			que->pop();
			
			socks->lock();

			vector<int> vec;
			for (auto iter = socks->begin(); iter != socks->end(); iter++) {

				int ret = send(iter->second, (char *)&tmp_msg.type, sizeof(int), 0);

				if (ret != sizeof(int)) {
					vec.push_back(iter->first);
				}

				else{
					send(iter->second, (char *)&tmp_msg.len, sizeof(int), 0);
					send(iter->second, tmp_msg.buff, tmp_msg.len, 0);
				}

			}

			for (int i = 0; i < vec.size(); ++i)
			{
				socks->erase(vec[i]);
			}
			vec.clear();
			socks->unlock();
		}

	}
}