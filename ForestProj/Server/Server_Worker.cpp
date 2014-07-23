#include <cstdio>
#include <string>
#include <vector>

#include "../protobuf/connect.pb.h"
#include "../protobuf/disconn.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"

#include "Completion_Port.h"
#include "types.h"
#include "Client_Map.h"
#include "msg.h"
#include "Sock_set.h"
#include "Memory_Pool.h"

#include <Windows.h>

using std::vector;
using std::string;

void set_single_cast(int, vector<int>&);
void set_multicast_in_room_except_me(Character*, vector<int>&, bool);
void send_message(msg, vector<int> &,bool);
void unpack(msg, char *, int *);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);

extern CRITICAL_SECTION cs;
int k;

//void Handler_PCONNECT(Character *pCharacter, Buffer* pBuf)
//{
//	CONNECT::CONTENTS connect;
//	INIT::CONTENTS initContents;
//	SET_USER::CONTENTS setuserContents;
//
//}

//void Handler_MOVE_USER(Character *pCharacter, Buffer* pBuf)
//{
//
//}

unsigned WINAPI Server_Worker(LPVOID pComPort)
{
	Client_Map *CMap = Client_Map::getInstance();
	Sock_set *sock_set = Sock_set::getInstance();

	auto HandlerPool = Handler_Pool::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();

	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	vector<int> receiver;

	while (true)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		if (ioInfo->RWmode == READ)
		{
			puts("MESSAGE RECEIVED!");
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printf("@Abnomal turn off ");
				remove_valid_client(handleInfo, ioInfo);
				continue;
			}

			//메시지를 받은 후 처리해야할 계산 부분

			int readbyte = 0;
			int type, len;

			{
				int *param[] = { &type, &len };
				copy_to_param(param, 2, ioInfo->buffer + readbyte);
				readbyte += 2 * sizeof(int);
			}
			std::string readContents(ioInfo->buffer + readbyte, len);

			if (type == PDISCONN) // 정상적인 종료의 경우
			{
				DISCONN::CONTENTS disconn;

				disconn.ParseFromString(readContents);
				if (disconn.data() != "BYE SERVER!")
				{
					//가짜 클라이언트
				}

				printf("Nomal turn off\n");
				remove_valid_client(handleInfo, ioInfo);
				continue;
			}
			else if (type == PCONNECT) // 새로 들어온 경우
			{
				CONNECT::CONTENTS connect;
				INIT::CONTENTS initContents;
				SET_USER::CONTENTS setuserContents;
				string bytestring;

				connect.ParseFromString(readContents);
				if (connect.data() != "HELLO SERVER!")
				{
					//가짜 클라이언트
				}

				int char_id;
				int x, y;
				static int id = 0;
				int writebyte;

				// 캐릭터 객체를 생성 후
				int copy_id = InterlockedIncrement((unsigned *)&id);
				Character* c = new Character(copy_id);
				char_id = copy_id;
				x = c->getX();
				y = c->getY();
				ioInfo->id = copy_id;
				ioInfo->myCharacter = c;

				bool isValid = false;
				while (true)
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

				// x와 y의 초기값을 가져온다.   
				initContents.clear_data();
				{
					auto myData = initContents.mutable_data()->Add();
					myData->set_id(char_id);
					myData->set_x(x);
					myData->set_y(y);
				}

				bytestring.clear();
				initContents.SerializeToString(&bytestring);
				len = bytestring.length();

				set_single_cast(char_id, receiver);
				send_message(msg(PINIT, len, bytestring.c_str()), receiver,true);
				receiver.clear();

				// 현재 접속한 캐릭터의 정보를 다른 접속한 유저들에게 전송한다.
				setuserContents.clear_data();// .clear_data();
				{
					auto myData = setuserContents.mutable_data()->Add();
					myData->set_id(char_id);
					myData->set_x(x);
					myData->set_y(y);
				}

				bytestring.clear();
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				/*setuserContents.ParseFromString(bytestring);
				printf("len : %d count : %d\n", len, setuserContents.data_size());
				for (int i = 0; i < setuserContents.data_size(); i++) {
				auto p = setuserContents.data(i);
				printf("id = %d, x = %d, y = %d\n", p.id(), p.x(), p.y());
				}*/

				set_multicast_in_room_except_me(c, receiver, true/*autolock*/);
				send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);
				receiver.clear();

				// PCONNECT로 접속한 유저에게 다른 객체들의 정보를 전송한다.
				setuserContents.clear_data();
				CMap->lock();
				for (auto itr = CMap->begin(); itr != CMap->end(); itr++)
				{
					if (itr->second == c) // 캐릭터 맵에 현재 들어간 내 객체의 정보를 보내려 할 때는 건너뛴다.
						continue;

					int tID = itr->second->getID();
					int tx = itr->second->getX();
					int ty = itr->second->getY();

					//if (tID == char_id) // 캐릭터 맵에 현재 들어간 내 객체의 정보를 보내려 할 때는 건너뛴다.
					//	continue;

					if (tx == x && ty == y)
					{
						auto tempData = setuserContents.mutable_data()->Add();
						tempData->set_id(tID);
						tempData->set_x(tx);
						tempData->set_y(ty);
					}
				}
				CMap->unlock();

				bytestring.clear();
				setuserContents.SerializeToString(&bytestring);
				len = bytestring.length();

				/*setuserContents.ParseFromString(bytestring);
				printf("len : %d count : %d\n", len, setuserContents.data_size());
				for (int i = 0; i < setuserContents.data_size(); i++) {
				auto p = setuserContents.data(i);
				printf("id = %d, x = %d, y = %d\n", p.id(), p.x(), p.y());
				}*/

				set_single_cast(char_id, receiver);
				send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);
				receiver.clear();
			}
			else if (type == PMOVE_USER)// 유저가 이동하는 경우
			{
				MOVE_USER::CONTENTS moveuserContents;
				ERASE_USER::CONTENTS eraseuserContents;
				SET_USER::CONTENTS setuserContents;
				std::string bytestring;

				moveuserContents.ParseFromString(readContents);

				int cur_id, x_off, y_off;
				int len;

				for (int i = 0; i < moveuserContents.data_size(); ++i)
				{
					auto user = moveuserContents.data(i);
					cur_id = user.id();
					x_off = user.xoff();
					y_off = user.yoff();

					/*CMap->lock();
					Character *now = CMap->find_id_to_char(cur_id);
					CMap->unlock();*/
					Character* now = ioInfo->myCharacter;

					// 기존의 방의 유저들의 정보를 삭제함
					CMap->lock();
					for (auto iter = CMap->begin(); iter != CMap->end(); iter++)
					{
						if (iter->second->getX() == now->getX() && iter->second->getY() == now->getY())
						{
							int nid = iter->second->getID();

							if (nid == now->getID())
								continue;

							auto eraseuser = eraseuserContents.add_data();
							eraseuser->set_id(nid);
						}
					}
					CMap->unlock();

					eraseuserContents.SerializeToString(&bytestring);
					len = bytestring.length();

					set_single_cast(now->getID(), receiver);
					send_message(msg(PERASE_USER, len, bytestring.c_str()), receiver, true);

					receiver.clear();
					bytestring.clear();
					eraseuserContents.clear_data();

					// 기존 방의 유저들에게 내가 사라짐을 알림
					auto eraseuser = eraseuserContents.add_data();
					eraseuser->set_id(cur_id);
					eraseuserContents.SerializeToString(&bytestring);
					len = bytestring.length();

					set_multicast_in_room_except_me(now, receiver, true/*autolock*/);
					send_message(msg(PERASE_USER, len, bytestring.c_str()), receiver, true);

					receiver.clear();
					bytestring.clear();
					eraseuserContents.clear_data();

					// 캐릭터를 해당 좌표만큼 이동시킴
					now->setX(now->getX() + x_off);
					now->setY(now->getY() + y_off);

					// 새로운 방의 유저들에게 내가 등장함을 알림
					int id = now->getID(), x = now->getX(), y = now->getY();
					auto setuser = setuserContents.add_data();
					setuser->set_id(id);
					setuser->set_x(x);
					setuser->set_y(y);

					setuserContents.SerializeToString(&bytestring);
					len = bytestring.length();

					set_multicast_in_room_except_me(now, receiver, true/*autolock*/);
					send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);

					receiver.clear();
					bytestring.clear();
					setuserContents.clear_data();

					// 새로운 방의 유저들의 정보를 불러옴

					CMap->lock();
					for (auto iter = CMap->begin(); iter != CMap->end(); iter++)
					{
						if (iter->second->getX() == x && iter->second->getY() == y)
						{
							int nid = iter->second->getID();
							int nx = iter->second->getX();
							int ny = iter->second->getY();

							auto setuser = setuserContents.add_data();
							setuser->set_id(nid);
							setuser->set_x(nx);
							setuser->set_y(ny);
						}
					}
					CMap->unlock();

					setuserContents.SerializeToString(&bytestring);
					len = bytestring.length();

					set_single_cast(now->getID(), receiver);
					send_message(msg(PSET_USER, len, bytestring.c_str()), receiver, true);

					receiver.clear();
					bytestring.clear();
					setuserContents.clear_data();

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
				if (WSAGetLastError() == WSA_IO_PENDING)
				{
				}
				else
				{
					// 소켓 에러 발생
				}
			}
		}
		else // WRITE
		{
			if (bytesTrans == 0) // 올바르지 않은 종류의 경우
			{
				printf("나 출력되는거 맞음?ㅋ\n");
				remove_valid_client(handleInfo, ioInfo);
				continue;
			}

			puts("MESSAGE SEND!");
//			free(ioInfo);
			ioInfoPool->pushBlock(ioInfo);
			printf("k Decrement %d\n", InterlockedDecrement((unsigned int *)&k));
		}
	}

	return 0;
}
