#include "iocp_server.h"

#include "iocp_error_codes.h"
#include "iocp_thread_worker.h"
#include "iocp_ov_socket.h"

#include "iocp_server_ctx.h"

int iocp_server_init(IOCP_SERVER_CTX **ctx, IOCP_SERVER_SETTINGS *settings, IOCP_SERVER_EVENT_LISTENER *event_listener)
{
	int error_level = 0;

	*ctx = malloc(sizeof(IOCP_SERVER_CTX));
	if (!*ctx)
	{
		error_level = IOCP_ERROR_ALLOC_CTX;
	}
	else
	{
		(*ctx)->iocp_handle					= INVALID_HANDLE_VALUE;
		(*ctx)->terminate_requested			= FALSE;
		(*ctx)->listener					= INVALID_SOCKET;
		(*ctx)->acceptor					= INVALID_SOCKET;
		(*ctx)->acceptor_buffer				= NULL;
		(*ctx)->bytes_received_on_accept	= 0;
		ZeroMemory(&(*ctx)->listen_ov, sizeof(OVERLAPPED));
		(*ctx)->threads						= NULL;
	}

	(*ctx)->settings		= *settings;
	(*ctx)->event_listener	= *event_listener;

	if (!error_level)
	{
		WSADATA wsa_data = { 0 };
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			error_level = IOCP_ERROR_WSA_STARTUP;
		}
	}

	if (!error_level)
	{
		(*ctx)->iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (!(*ctx)->iocp_handle)
		{
			error_level = IOCP_ERROR_CREATE_IOCP;
		}
	}		

	if (!error_level)
	{
		if (!(*ctx)->settings.get_status_timeout)
		{
			(*ctx)->settings.get_status_timeout = IOCP_GET_STATUS_TIMEOUT_DEFAULT;
		}
	}	

	if (!error_level)
	{
		(*ctx)->listener = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if ((*ctx)->listener == INVALID_SOCKET)
		{
			error_level = IOCP_ERROR_CREATE_LISTEN_SOCKET;
		}
	}	

	if (!error_level)
	{
		SOCKADDR_IN bind_addr = { 0 };
		bind_addr.sin_family = AF_INET;

		if (!(*ctx)->settings.port)
		{
			(*ctx)->settings.port = IOCP_PORT_DEFAULT;
		}

		bind_addr.sin_port = htons((*ctx)->settings.port);

		if (!lstrlenW((*ctx)->settings.ip))
		{
			StringCchCopyW((*ctx)->settings.ip, IOCP_IP_ADDRESS_LENGTH, IOCP_IPADDRESS_DEFAULT);
		}

		if (!InetPtonW(AF_INET, (*ctx)->settings.ip, &bind_addr.sin_addr))
		{
			error_level = IOCP_ERROR_CONVERT_IP_ADDRESS;
		}

		if (!error_level)
		{
			if (bind((*ctx)->listener, (const SOCKADDR *)&bind_addr, sizeof(SOCKADDR)) == SOCKET_ERROR)
			{
				error_level = IOCP_ERROR_BIND;
			}
		}
	}	

	if (!error_level)
	{
		if (listen((*ctx)->listener, SOMAXCONN) == SOCKET_ERROR)
		{
			error_level = IOCP_ERROR_LISTEN;
		}
	}
	
	if (!error_level)
	{
		(*ctx)->acceptor = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if ((*ctx)->acceptor == INVALID_SOCKET)
		{
			error_level = IOCP_ERROR_CREATE_ACCEPTOR_SOCKET;
		}
	}
	
	if (!(*ctx)->settings.buffers_size)
	{
		(*ctx)->settings.buffers_size = IOCP_BUFFERS_SIZE_DEFAULT;
	}

	if (!error_level)
	{
		(*ctx)->acceptor_buffer = (char *)malloc((*ctx)->settings.buffers_size);
		if (!(*ctx)->acceptor_buffer)
		{
			error_level = IOCP_ERROR_ALLOC_ACCEPTOR_BUFFER;
		}		
	}

	if (!error_level)
	{
		error_level = accept_pending(*ctx);
	}

	if (!error_level)
	{
		HANDLE new_iocp_handle = CreateIoCompletionPort((HANDLE)(*ctx)->listener, (*ctx)->iocp_handle, (ULONG_PTR)&(*ctx)->listen_ov, 0);
		if (!new_iocp_handle)
		{
			error_level = IOCP_ERROR_CREATE_IOCP_WITH_LISTENER;
		}
		else
		{
			(*ctx)->iocp_handle = new_iocp_handle;
		}
	}	

	if (!error_level)
	{
		if (!(*ctx)->event_listener.on_server_init_fptr)
		{
			error_level = IOCP_ERROR_ON_SERVER_INIT_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!(*ctx)->event_listener.on_server_terminate_fptr)
		{
			error_level = IOCP_ERROR_ON_SERVER_TERMINATE_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!(*ctx)->event_listener.on_connected_fptr)
		{
			error_level = IOCP_ERROR_ON_CONNECTED_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!(*ctx)->event_listener.on_disconnected_fptr)
		{
			error_level = IOCP_ERROR_ON_DISCONNECTED_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!(*ctx)->event_listener.on_received_fptr)
		{
			error_level = IOCP_ERROR_ON_RECEIVED_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!(*ctx)->event_listener.on_sended_fptr)
		{
			error_level = IOCP_ERROR_ON_SENDED_FPTR_NOT_SET;
		}
	}		

	if (!error_level)
	{
		if (!(*ctx)->settings.threads_count)
		{
			SYSTEM_INFO sys_info = { 0 };
			GetSystemInfo(&sys_info);
			(*ctx)->settings.threads_count = (int)sys_info.dwNumberOfProcessors;
		}

		(*ctx)->threads = (HANDLE *)malloc((SIZE_T)(*ctx)->settings.threads_count * sizeof(HANDLE));
		if (!(*ctx)->threads)
		{
			error_level = IOCP_ERROR_ON_SENDED_FPTR_NOT_SET;
		}
	}	

	if (!error_level)
	{
		SIZE_T i;
		for (i = 0; i < (SIZE_T)(*ctx)->settings.threads_count && !error_level; ++i)
		{
			(*ctx)->threads[i] = CreateThread(NULL, 0, &iocp_thread_worker, *ctx, 0, NULL);
			if (!(*ctx)->threads[i])
			{
				error_level = IOCP_ERROR_CREATE_THREAD;
			}
		}
	}

	if (error_level)
	{
		iocp_server_terminate(ctx);
	}
	else
	{
		(*ctx)->event_listener.on_server_init_fptr();
	}

	return error_level;
}

void iocp_server_terminate(IOCP_SERVER_CTX **ctx)
{
	(*ctx)->terminate_requested = TRUE;

	(*ctx)->event_listener.on_server_terminate_fptr();

	if ((*ctx)->settings.threads_count)
	{
		WaitForMultipleObjects((DWORD)(*ctx)->settings.threads_count, (*ctx)->threads, TRUE, INFINITE);
	}	

	if ((*ctx)->threads)
	{
		SIZE_T i;
		for (i = 0; i < (SIZE_T)(*ctx)->settings.threads_count; ++i)
		{
			CloseHandle((*ctx)->threads[i]);
		}

		free((*ctx)->threads);

		(*ctx)->threads = NULL;
	}

	if ((*ctx)->acceptor_buffer)
	{
		free((*ctx)->acceptor_buffer);
		(*ctx)->acceptor_buffer = NULL;
	}

	if ((*ctx)->acceptor != INVALID_SOCKET)
	{
		closesocket((*ctx)->acceptor);
		(*ctx)->acceptor = INVALID_SOCKET;
	}
	
	if ((*ctx)->listener != INVALID_SOCKET)
	{
		closesocket((*ctx)->listener);
		(*ctx)->acceptor = INVALID_SOCKET;
	}	

	if ((*ctx)->iocp_handle)
	{
		CloseHandle((*ctx)->iocp_handle);
		(*ctx)->iocp_handle = INVALID_HANDLE_VALUE;
	}

	WSACleanup();

	*ctx = NULL;
}
