#pragma once

#include "iocp_constants.h"

#define IOCP_BUFFERS_SIZE_DEFAULT		8196
#define IOCP_PORT_DEFAULT				7777
#define IOCP_IPADDRESS_DEFAULT			L"0.0.0.0"
#define IOCP_GET_STATUS_TIMEOUT_DEFAULT 1000

typedef struct _IOCP_SERVER_SETTINGS
{
	int						get_status_timeout;
	unsigned short			port;
	wchar_t					ip[IOCP_IP_ADDRESS_LENGTH];
	int						buffers_size;
	int						threads_count;
} IOCP_SERVER_SETTINGS;
