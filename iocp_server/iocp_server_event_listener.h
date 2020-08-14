#pragma once

typedef void (on_server_init_func)();
typedef void (on_server_terminate_func)();
typedef void (on_connected_func)(void *);
typedef void (on_disconnected_func)(void *);
typedef void (on_received_func)(void *);
typedef void (on_sended_func)(void *);

typedef struct _IOCP_SERVER_EVENT_LISTENER
{
	on_server_init_func			*on_server_init_fptr;
	on_server_terminate_func	*on_server_terminate_fptr;
	on_connected_func			*on_connected_fptr;
	on_disconnected_func		*on_disconnected_fptr;
	on_received_func			*on_received_fptr;
	on_sended_func				*on_sended_fptr;
} IOCP_SERVER_EVENT_LISTENER;
