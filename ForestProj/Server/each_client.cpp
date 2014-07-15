//#include <WinSock2.h>
//
//#include <iostream>
//#include <mutex>
//#include <map>
//
//#include "que.h"
//#include "msg.h"
//#include "types.h"
//#include "Disc_user_map.h"
//#include "Client_Map.h"
//#include "Synched_list.h"
//
//using namespace std;
//
//#define END_MSG "\\QUIT"
//
//void each_client() {
//	SYNCHED_QUEUE *que = SYNCHED_QUEUE::getInstance();
//	Disc_User_Map *Disc_User = Disc_User_Map::getInstance();
//	Client_Map *CMap = Client_Map::getInstance();
//	Synched_List *List = Synched_List::getInstance();
//
//	int type;
//	int len;
//	char Buff[1024];
//
//	do
//	{
//		for (auto itr = List->begin(); itr != List->end();)
//		{
//			SOCKET Connection = *itr;
//			int ret = recv(Connection, (char *)&type, sizeof(int), 0);
//			int nError = WSAGetLastError();
//			//printf("nerror : %d\n", nError);
//			if (nError == WSAEWOULDBLOCK )
//			{
//				// 소켓에서 메시지를 아직 받지 못한 경우
//				//printf("nerror : %d\n", nError);
//				// 건너뜁시다!! 아직 안온거니까
//				itr++;
//			}
//			else if (nError == 0)
//			{
//				// 정상적으로 메시지 수신이 완료된 경우
//
//				if ( type == DISCONN ) // 정상적인 종료의 경우
//				{
//					//printf("User %d(Socket : %d) is disconnected\n", User_ID, Connection);
//					char *pBuf = Buff;
//					memcpy(pBuf, &Connection, sizeof(SOCKET));
//					pBuf += sizeof(SOCKET);
//					memcpy(pBuf, "BYE SERVER!", sizeof("BYE SERVER!"));
//					pBuf += sizeof("BYE SERVER!");
//
//					len = pBuf - Buff;
//					CMap->lock();
//					int char_id = CMap->find_sock_to_id(Connection);
//					CMap->unlock();
//
//					Disc_User->insert(pair<SOCKET, int>(Connection, char_id));
//					que->push(msg(DISCONN, len, Buff));
//
//					auto itr2 = itr;
//					itr2++;
//
//					List->erase(itr);
//					closesocket(Connection); // 연결 소켓을 닫는다.
//
//					itr = itr2;
//				}
//				else if (type == CONNECT) // 새로 들어온 경우
//				{
//					for (int inc = 0; inc < sizeof(int);)
//					{
//						int ret = recv(Connection, (char *)&len+inc, sizeof(int), 0);
//						auto tError = WSAGetLastError();
//
//						if ( tError != WSAEWOULDBLOCK )
//						{
//							inc += ret;
//						}
//					}
//					
//					char* pBuf = Buff;
//					memcpy(pBuf, &Connection, sizeof(SOCKET));
//					pBuf += sizeof(SOCKET);
//
//					for (int inc = 0; inc < len;)
//					{
//						int ret = recv(Connection, pBuf + inc, len - inc, 0);
//						if (ret != SOCKET_ERROR)
//						{
//							inc += ret;
//						}
//					}
//
//					pBuf += len;
//					len += sizeof(SOCKET);
//					*pBuf = '/0';
//					que->push(msg(type, len, Buff));
//					itr++;
//				}
//				else // 나머지 문자의 경우
//				{
//					for (int inc = 0; inc < sizeof(int);)
//					{
//						int ret=recv(Connection, (char *)&len, sizeof(int), 0);
//						if (ret != SOCKET_ERROR)
//						{
//							inc += ret;
//						}
//					}
//					char* pBuf = Buff;
//					printf("len : %d\n", len);
//					for (int inc = 0; inc < len;)
//					{
//						int ret = recv(Connection, pBuf + inc, len - inc, 0);
//						if (ret != SOCKET_ERROR)
//						{
//							inc += ret;
//						}
//					}
//
//					que->push(msg(type, len, Buff));
//					itr++;
//				}
//			}
//			else
//			{
//				// 비정상적인 종료의 경우
//				char *pBuf = Buff;
//				memcpy(pBuf, &Connection, sizeof(SOCKET));
//				pBuf += sizeof(SOCKET);
//				memcpy(pBuf, "BYE SERVER!", sizeof("BYE SERVER!"));
//				pBuf += sizeof("BYE SERVER!");
//
//				len = pBuf - Buff;
//				CMap->lock();
//				int char_id = CMap->find_sock_to_id(Connection);
//				CMap->unlock();
//
//				Disc_User->insert(pair<SOCKET, int>(Connection, char_id));
//				que->push(msg(DISCONN, len, Buff));
//
//				auto itr2 = itr;
//				itr2++;
//				List->erase(itr);
//				closesocket(Connection); // 연결 소켓을 닫는다.
//
//				itr = itr2;
//			}
//		}
//		Sleep(1);
//	} while (true);
//}