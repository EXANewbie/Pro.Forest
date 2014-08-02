#include <cstdio>
#include <conio.h>
#include <WinSock2.h>
#include <string>

#include "character.h"
#include "cmap.h"
#include "types.h"
#include "Scoped_Lock.h"
#include "mmap.h"

#include "../protobuf/connect.pb.h""
#include "../protobuf/disconn.pb.h"
#include "../protobuf/eraseuser.pb.h"
#include "../protobuf/init.pb.h"
#include "../protobuf/moveuser.pb.h"
#include "../protobuf/setuser.pb.h"
#include "../protobuf/setmonster.pb.h"
#include "../protobuf/erasemonster.pb.h"
#include "../protobuf/userattack.pb.h"
#include "../protobuf/userattackresult.pb.h"

void send_move(const SOCKET s, const char& c, const int& myID);
void copy_to_buffer(char *buf, int* type, int* len, std::string* content);
void receiver(const SOCKET s, int* myID);

void Sender(const SOCKET sock, int* myID, Character* myChar)
{
	SYNCHED_CHARACTER_MAP* chars = SYNCHED_CHARACTER_MAP::getInstance();
	SYNCHED_MONSTER_MAP* mons = SYNCHED_MONSTER_MAP::getInstance();

	int type;
	int len;
	char *buf = new char[1024000];

	while (true)
	{
		char c;
		c = _getch();
		if (c == 'x')
		{
			//PDISCONN ����
			type = PDISCONN;

			DISCONN::CONTENTS contents;
			std::string* buff_msg = contents.mutable_data();
			*buff_msg = "BYE SERVER!";
			std::string bytestring;
			contents.SerializeToString(&bytestring);

			len = bytestring.length();

			copy_to_buffer(buf, &type, &len, &bytestring);

			send(sock, buf, len + sizeof(int)* 2, 0);
			break;
		}
		else if (c == 'w' || c == 'a' || c == 's' || c == 'd')
		{
			send_move(sock, c, *myID);
		}
		else if (c == 'i')
		{
			//������ ��������.
			printf("##�� ����:\n");
			if (myChar->getID() == -1)
			{
				printf("���� ĳ���Ͱ� �������� �ʾҽ��ϴ�.\n��ø� ��ٷ� �ֽʽÿ�\n\n");
				continue;
			}
			{
				Scoped_Rlock SR(myChar->getLock());
				printf("ID : %d ��ġ : %d, %d\n���� : %d ü��(����/�ְ�) : %d/%d ���ݷ� : %d\n\n", 
					myChar->getID(), myChar->getX(), myChar->getY(), myChar->getLv(), myChar->getPrtHp(), myChar->getMaxHp(), myChar->getPower());
			}
		}
		else if (c == 'j')
		{
			//���� �濡 �ִ� �������� ������ ��������.
			printf("##���� ����:\n");
			{
				Scoped_Rlock SR(&chars->srw);
				int size = chars->size();
				if (size == 1)
				{
					printf("���� �����濡 ���ᰡ �����ϴ�\n\n");
					continue;
				}
				printf("--����� %d��\n", size - 1);
				for (auto iter = chars->begin(); iter != chars->end(); ++iter)
				{
					Character* other = iter->second;
					if (other->getID() == *myID) continue;
					printf("ID : %d ��ġ : %d, %d\n���� : %d ü��(����/�ְ�) : %d/%d ���ݷ� : %d\n\n",
						other->getID(), other->getX(), other->getY(), other->getLv(), other->getPrtHp(), other->getMaxHp(), other->getPower());
				}
			}
		}
		else if (c == 'o')
		{
			//���� �濡 �ִ� ���͵��� ������ ��������.
			{
				Scoped_Rlock SR(&mons->srw);
				int size = mons->size();
				if (size == 0)
				{
					printf("���� �����濡 ���Ͱ� �����ϴ�\n\n");
					continue;
				}
				printf("--���� �� %d����\n", size);
				for (auto iter = mons->begin(); iter != mons->end(); ++iter)
				{
					Monster* mon = iter->second;
					printf("�̸�(ID) : %s (%d) ��ġ : %d, %d\n���� : %d ü��(����/�ְ�) : %d/%d ���ݷ� : %d\n\n",
						mon->getName().c_str(), mon->getID(), mon->getX(), mon->getY(), mon->getLv(), mon->getPrtHp(), mon->getMaxHp(), mon->getPower());
				}
			}
		}
		else if (c == 'q')
		{
			if (myChar->getID() == -1)
			{
				printf("���� ĳ���Ͱ� �������� �ʾҽ��ϴ�.\n��ø� ��ٷ� �ֽʽÿ�\n\n");
				continue;
			}
			// ������ �ϵ��� ����!
			Monster* atkMon;
			{
				Scoped_Rlock SR(&mons->srw);
				int size = mons->size();
				if (size == 0)
				{
					printf("���� �����濡 ������ ���Ͱ� �����ϴ�\n\n");
					continue;
				}
				//���� ü���� ���ѳ���� ������´�!
				Monster* weakMon = NULL;
				int weakHp = 987654321;
				
				for (auto iter = mons->begin(); iter != mons->end(); ++iter)
				{
					if (weakHp > iter->second->getPrtHp())
					{
						weakMon = iter->second;
						weakHp = weakMon->getPrtHp();
					}					
				}
				atkMon = weakMon;
			}
			
			USER_ATTACK::CONTENTS userattackContents;
			auto userattack = userattackContents.add_data();
			/*{
				Scoped_Rlock SR(myChar->getLock());
				userattack->set_id(myChar->getID());
				userattack->set_x(myChar->getX());
				userattack->set_y(myChar->getY());
			}*/
			userattack->set_attcktype(1);
			{
				Scoped_Rlock SR(atkMon->getLock());
				userattack->set_id_m(atkMon->getID());
				userattack->set_x(atkMon->getX());
				userattack->set_y(atkMon->getY());
			}
			std::string bytestring;
			userattackContents.SerializeToString(&bytestring);
			type = PUSER_ATTCK;
			len = bytestring.length();
			copy_to_buffer(buf, &type, &len, &bytestring);

			if (send(sock, buf, len + sizeof(int)* 2, 0) == SOCKET_ERROR)
			{
				printf("�ý��� ����\n");
			}
			else
			{
				printf("���� %s[%d]�� �����Ͽ����ϴ�!\n", atkMon->getName().c_str(), atkMon->getID());
			}
		}
	}
}