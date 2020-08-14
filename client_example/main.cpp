#include <WS2tcpip.h>

#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

unsigned long __stdcall thread_worker(void *params);

int main()
{
	WSAData wsa_data = {};
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

	for (size_t i = 0; i < 65535; ++i)
	{
		CreateThread(NULL, 0, &thread_worker, NULL, 0, NULL);
	}

	for (;;)
	{		
		if (_getch() == 27)
		{
			break;
		}		
	}

	WSACleanup();

	return 0;
}

unsigned long __stdcall thread_worker(void *params)
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN dest_addr = {};
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(7777);
	InetPton(AF_INET, TEXT("127.0.0.1"), &dest_addr.sin_addr);
	connect(s, (const SOCKADDR *)&dest_addr, sizeof(SOCKADDR_IN));

	int res = send(s, "0", 1, 0);
	char recv_buffer[1] = {};
	res = recv(s, recv_buffer, 1, 0);

	Sleep(5000);

	shutdown(s, SD_BOTH);
	closesocket(s);

	return 0;
}
