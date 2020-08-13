#pragma once

#include "iocp_constants.h"

typedef struct _IOCP_SERVER_SETTINGS
{
	int						get_status_timeout;
	unsigned short			port;
	wchar_t					ip[IOCP_IP_ADDRESS_LENGTH];
	int						buffers_size;
	int						threads_count;
} IOCP_SERVER_SETTINGS;
