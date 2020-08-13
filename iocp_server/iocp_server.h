#pragma once

#include <wchar.h>

#include "iocp_server_settings.h"
#include "iocp_server_event_listener.h"

#include "iocp_connection.h"

typedef struct _IOCP_SERVER_CTX IOCP_SERVER_CTX;

#ifdef __cplusplus
extern "C"
{
#endif
	int iocp_server_init(IOCP_SERVER_CTX **ctx, IOCP_SERVER_SETTINGS *settings, IOCP_SERVER_EVENT_LISTENER *event_listener);
	void iocp_server_terminate(IOCP_SERVER_CTX **ctx);
#ifdef __cplusplus
}
#endif
