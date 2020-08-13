#pragma once

#include "win32_networking.h"

#include "iocp_server_settings.h"
#include "iocp_server_event_listener.h"

typedef struct _IOCP_SERVER_CTX
{
	volatile BOOL				terminate_requested;
	HANDLE						iocp_handle;	
	SOCKET						listener;
	SOCKET						acceptor;
	char						*acceptor_buffer;
	DWORD						bytes_received_on_accept;
	OVERLAPPED					listen_ov;
	HANDLE						*threads;
	IOCP_SERVER_SETTINGS		settings;
	IOCP_SERVER_EVENT_LISTENER	event_listener;
} IOCP_SERVER_CTX;
