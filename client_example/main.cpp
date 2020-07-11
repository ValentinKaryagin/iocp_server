#include <WS2tcpip.h>

#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

unsigned long __stdcall thread_worker(void *params);

int main()
{
	WSAData wsa_data{};
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

	for (size_t i = 0; i < 32; ++i)
	{
		CreateThread(NULL, 0, &thread_worker, NULL, 0, NULL);
	}

	for (;;)
	{
		Sleep(1000);
	}

	WSACleanup();

	return 0;
}

unsigned long __stdcall thread_worker(void *params)
{
	printf("thread begin\n");

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN dest_addr{};
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(25565);
	InetPton(AF_INET, TEXT("127.0.0.1"), &dest_addr.sin_addr);
	connect(s, (const SOCKADDR *)&dest_addr, sizeof(SOCKADDR_IN));

	for (size_t i = 0; i < 1000000; ++i)
	{
		send(s, "0123456789", 10, 0);

		char recv_buffer[10]{};

		recv(s, recv_buffer, 10, 0);
	}	

	closesocket(s);

	printf("thread end\n");

	return 0;
}
