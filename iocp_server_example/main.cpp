#include "event_listener.h"

#include <conio.h>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

int main()
{
	int error_level = 0;

	IOCP_SERVER_SETTINGS settings = {};

	IOCP_SERVER_EVENT_LISTENER event_listener = {};
	event_listener.on_server_init_fptr		= &on_server_init;
	event_listener.on_server_terminate_fptr = &on_server_terminate;
	event_listener.on_connected_fptr		= &on_connected;
	event_listener.on_disconnected_fptr		= &on_disconnected;
	event_listener.on_received_fptr			= &on_received;
	event_listener.on_sended_fptr			= &on_sended;

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
