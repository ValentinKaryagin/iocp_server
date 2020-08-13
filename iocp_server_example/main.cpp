#include "..\iocp_server\iocp_server.h"

#include <Windows.h>

#include <conio.h>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

void on_connected(void *handle);
void on_disconnected(void *handle);
void on_received(void *handle);
void on_sended(void *handle);

int main()
{
	int error_level = 0;

	IOCP_SERVER_SETTINGS settings = {};

	IOCP_SERVER_EVENT_LISTENER event_listener = {};
	event_listener.on_connected_fptr	= &on_connected;
	event_listener.on_disconnected_fptr	= &on_disconnected;
	event_listener.on_received_fptr		= &on_received;
	event_listener.on_sended_fptr		= &on_sended;

	IOCP_SERVER_CTX *ctx = NULL;
	error_level = iocp_server_init(&ctx, &settings, &event_listener);
	if (!error_level)
	{
		for (;;)
		{
			if (_getch() == 27)
			{
				break;
			}
		}
	}

	if (ctx)
	{
		iocp_server_terminate(&ctx);
	}	

	return error_level;
}

void on_connected(void *handle)
{
	//printf(__FUNCTION__ "()\n");
}

void on_disconnected(void *handle)
{
	//printf(__FUNCTION__ "()\n");
}

void on_received(void *handle)
{
	int				data_size	= 0;
	unsigned char	*data_ptr	= NULL;
	
	iocp_connection_get_recv_data(handle, &data_size, &data_ptr);

	iocp_connection_set_send_data(handle, data_size, data_ptr);
	iocp_connection_send(handle);

	//printf(__FUNCTION__ "() -> size: [%d] data: [%s]\n", data_size, (char *)data_ptr);
}

void on_sended(void *handle)
{
	//printf(__FUNCTION__ "()\n");

	iocp_connection_disconnect(handle);
}
