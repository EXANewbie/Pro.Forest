#include <WinSock2.h>
#include <cstdarg>
#include <vector>

#include "../protobuf/eraseuser.pb.h"

#include "Check_Map.h"
#include "types.h"
#include "Completion_Port.h"
#include "msg.h"
#include "character.h"
#include "Sock_set.h"
#include "Memory_Pool.h"
#include "DMap.h"
#include "Scoped_Lock.h"

#include "monster.h"
#include "DMap_monster.h"

using namespace std;

extern int k;

void printLog(const char *msg, ...);
void set_single_cast(int, vector<int>&);
void make_vector_id_in_room_except_me(Character*, vector<int>&, bool);
void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked);
void send_message(msg, vector<Character *> &, bool);
void unpack(msg, char *, int *);
void closeClient(int);
void remove_valid_client(LPPER_HANDLE_DATA, LPPER_IO_DATA);
void copy_to_buffer(char *, int **, int);
void copy_to_param(int **, int, char *);


bool Boundary_Check(const int, const int, int, int);
int bigRand();

void set_single_cast(Character* c, vector<Character *>& send_list)
{
	send_list.push_back(c);
}

void make_vector_id_in_room_except_me(Character* myChar, vector<Character *>& send_list, bool autolocked)
{
	auto FVEC = F_Vector::getInstance();
	E_List* elist = FVEC->get(myChar->getX(), myChar->getY());

	if (autolocked == true)
	{
		AcquireSRWLockShared(&elist->slock);
	}
	
	for (auto itr = elist->begin(); itr != elist->end(); itr++)
	{
		if (myChar->getID() != (*itr)->getID())
		{
			send_list.push_back(*itr);
		}
	}

	if (autolocked == true)
	{
		ReleaseSRWLockShared(&elist->slock);
	}
}

void make_monster_vector_in_room(Character* myChar, vector<Monster *>& send_list, bool autolocked)
{
	auto FVEC = F_Vector_Mon::getInstance();
	E_List_Mon* elist = FVEC->get(myChar->getX(), myChar->getY());

	if (autolocked == true)
	{
		AcquireSRWLockShared(&elist->slock);
	}

	for (auto itr = elist->begin(); itr != elist->end(); itr++)
	{
		send_list.push_back(*itr);
	}

	if (autolocked == true)
	{
		ReleaseSRWLockShared(&elist->slock);
	}
}

void send_message(msg message, vector<Character *> &send_list, bool autolocked) {
	auto CMap = Check_Map::getInstance();
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();
//	Client_Map *CMap = Client_Map::getInstance();
	//vector< pair<int,SOCKET> > errors;

	int len;

	for (int i = 0; i < send_list.size(); i++)
	{
		//auto id = send_list[i]->getID();
		SOCKET sock = send_list[i]->getSock();

		/*반드시 여기에 sock의 현재 ID의 owner가 id인지 체크하는 로직을 작성해야 한다.*/
		/*만약 아닐 경우, 해당 소켓은 다른 클라이언트가 접속한 것이므로 건너뛰도록 한다.*/
		/* if( Sock_Map.find_id(sock) != id ) continue;*/
		if (CMap->check(sock, send_list[i]->getID())==false)
		{
			continue;
		}

		LPPER_IO_DATA ioInfo = ioInfoPool->popBlock();
		ioInfo->block = MemoryPool->popBlock();

		unpack(message, ioInfo->block->getBuffer(), &len);
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = len;
		ioInfo->wsaBuf.buf = ioInfo->block->getBuffer();
		ioInfo->RWmode = WRITE;

		int ret = WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

		if (ret == SOCKET_ERROR)
		{
			if (WSAGetLastError() == ERROR_IO_PENDING)
			{
				printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
				// 큐에 들어감 ^.^
			}
			else
			{
				// 너에겐 수많은 이유가 있겠지... 하지만 아마도 그 수많은 이유들의 공통점은 소켓에 전송할 수 없는 것이 아닐까?
				if (ioInfo->block != nullptr) {
					MemoryPool->pushBlock(ioInfo->block);
					ioInfo->block = nullptr;
				}
				ioInfoPool->pushBlock(ioInfo);
				printLog("Send Error (%d)\n", WSAGetLastError());
				//free(ioInfo);
			}
		}
		else
		{
			printLog("k Increment %d\n", InterlockedIncrement((unsigned int *)&k));
		}
	}
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

void closeClient(SOCKET sock, int id, Character* myChar)
{
	auto FVEC = F_Vector::getInstance();
	auto elist = FVEC->get(myChar->getX(), myChar->getY());
	auto Amap = Access_Map::getInstance();

	vector<Character *> send_list;

	int ret = closesocket(sock);

	if (ret != WSAENOTSOCK)
	{
		ERASE_USER::CONTENTS contents;
		contents.add_data()->set_id(id);

		std::string bytestring;
		contents.SerializeToString(&bytestring);
		{
			Scoped_Wlock SW1(&Amap->slock);
			Scoped_Wlock SW2(&elist->slock);

			// 처음으로 소켓을 닫을 때.
			make_vector_id_in_room_except_me(myChar, send_list, false/*not autolock*/);

			//나의 캐릭터를 지우라고 같은 방 식구들에게 통보.
			send_message(msg(PERASE_USER, sizeof(int), bytestring.c_str()), send_list, false);

			//캐릭터관리 맵에서 캐릭터를 빼고 있음.
			Amap->erase(id);
			elist->erase(myChar);
		}
		
		//이제 나의 캐릭터를 진짜로 지운다.
		delete myChar;
	}
	else
	{
		//이미 삭제된 소켓.
	}
}

void remove_valid_client(LPPER_HANDLE_DATA handleInfo, LPPER_IO_DATA ioInfo)
{
	auto ioInfoPool = ioInfo_Pool::getInstance();
	auto HandlerPool = Handler_Pool::getInstance();
	auto MemoryPool = Memory_Pool::getInstance();
	auto CMap = Check_Map::getInstance();

	if (ioInfo->id == NOT_JOINED) // 현재 유저가 PCONNECT를 보내지 않은 상태일 경우
	{
		closesocket(handleInfo->hClntSock);
		Sock_set::getInstance()->erase(handleInfo->hClntSock);

		if (ioInfo->block != nullptr) {
			MemoryPool->pushBlock(ioInfo->block);
			ioInfo->block = nullptr;
		}
		HandlerPool->pushBlock(handleInfo);
		ioInfoPool->pushBlock(ioInfo);
		return;
	}

	if (CMap->check(handleInfo->hClntSock,ioInfo->id) == false)
	{
		// 이미 삭제 처리 된 경우를 여기에 명시한다.
	}
	else
	{
		CMap->erase(handleInfo->hClntSock);
		printLog("sock : %d char_id : %d\n", handleInfo->hClntSock, ioInfo->id);
		closeClient(handleInfo->hClntSock, ioInfo->id, ioInfo->myCharacter);
	}
	if (ioInfo->block != nullptr) {
		MemoryPool->pushBlock(ioInfo->block);
		ioInfo->block = nullptr;
	}
	HandlerPool->pushBlock(handleInfo);
	ioInfoPool->pushBlock(ioInfo);
}

void copy_to_buffer(char *buf, int *param[], int count)
{
	int writebyte = 0;
	for (int i = 0; i < count; i++)
	{
		memcpy(buf + writebyte, param[i], sizeof(int));
		writebyte += sizeof(int);
	}
}

void copy_to_param(int *param[], int count, char *buf)
{
	int readbyte = 0;
	for (int i = 0; i < count; i++)
	{
		memcpy(param[i], buf + readbyte, sizeof(int));
		readbyte += sizeof(int);
	}
}

void printLog(const char *msg, ...)
{
#ifdef PRINT_LOG
	const int BUF_SIZE = 512;
	char buf[BUF_SIZE] = { 0, };
	va_list ap;

	strcpy_s(buf, "Log : ");
	va_start(ap, msg);
	vsprintf_s(buf + strlen(buf),BUF_SIZE-strlen(buf), msg, ap);
	va_end(ap);

	puts(buf);
#endif;
}

bool Boundary_Check(int id, const int cX, const int cY, int x_off, int y_off)
{
	bool checker = true;
	if (cX + x_off < 0)
	{
		printLog("User(%d) try to move left at left boundary.", id);
		checker = false;
	}
	if (cX + x_off > WIDTH)
	{
		printLog("User(%d) try to move right at right boundary.", id);
		checker = false;
	}
	if (cY + y_off < 0)
	{
		printLog("User(%d) try to move up at top boundary.", id);
		checker = false;
	}
	if (cY + y_off > HEIGHT)
	{
		printLog("User(%d) try to move down at bottom boundary.", id);
		checker = false;
	}

	return checker;
}

int bigRand()
{
	return (rand() << 15) + rand();
}