#include "..\iocp_server\iocp_server.h"

#include <Windows.h>

#include <cstdio>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

void on_connect(void *handle);
void on_disconnect(void *handle);
void on_recv(void *handle);
void on_send(void *handle);

int main()
{
	int error_level = 0;

	IOCP_SERVER_INIT_INFO init_info{};
	init_info.on_connect_fptr		= &on_connect;
	init_info.on_disconnect_fptr	= &on_disconnect;
	init_info.on_recv_fptr			= &on_recv;
	init_info.on_send_fptr			= &on_send;

	error_level = iocp_server_init(&init_info);
	if (!error_level)
	{
		for (;;)
		{
			Sleep(1000);
		}
	}

	iocp_server_terminate();

	return error_level;
}

void on_connect(void *handle)
{
	printf(__FUNCTION__ "()\n");
}

void on_disconnect(void *handle)
{
	printf(__FUNCTION__ "()\n");
}

void on_recv(void *handle)
{
	int				data_size	= 0;
	unsigned char	*data_ptr	= NULL;
	
	iocp_connection_get_recv_data(handle, &data_size, &data_ptr);

	iocp_connection_set_send_data(handle, data_size, data_ptr);
	iocp_connection_send(handle);

	printf(__FUNCTION__ "() -> size: [%d] data: [%s]\n", data_size, (char *)data_ptr);
}

void on_send(void *handle)
{
	printf(__FUNCTION__ "()\n");

	iocp_connection_disconnect(handle);
}
