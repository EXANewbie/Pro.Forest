#include <cstdio>
#include <WinSock2.h>

#define PORT 78911
#define SERVER_IP_ADDRESS "10.1.7.206"

void main(void)
{

	WSADATA wsaData;
	SOCKET s;
	SOCKADDR_IN ServerAddr;


	// ���� 2.2�� �ʱ�ȭ

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// Ŭ���̾�Ʈ�� ���� ����

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	printf("%d\n", WSAGetLastError());

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

	// ������ ����

	connect(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));


	printf("%d\n", WSAGetLastError());
	// ������ �۽� �κ�
	printf("connection success");
	char buf[1024] = "hihi LJH";
	
	int t = 10;
	while (t--)
	{
		int len = strlen(buf);
		send(s, (char*)&len, sizeof(int), 0); // message header transfer
		int a=send(s, buf, len, 0);//message body transgfer
	}
	//end ����.
	int ptr;
	strncpy_s(buf, "end", sizeof("end"));
	ptr = strlen(buf);
	send(s, (char *)&ptr, sizeof(int), 0);
	send(s, buf, ptr, 0);

	printf("�޽��� ��ٸ�");
	recv(s, (char *)&ptr, sizeof(int), 0);
	recv(s, buf, ptr, 0);
	buf[ptr] = 0;

	printf("%s\n", buf);
	printf("%d\n", WSAGetLastError());

	closesocket(s);

}