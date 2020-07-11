#include "iocp_server.h"

#include <WS2tcpip.h>
#include <MSWSock.h>
#include <strsafe.h>

typedef struct _OV_SOCKET
{
	SOCKET s;
	char *raw_recv_buffer, *raw_send_buffer;
	WSABUF recv_buffer, send_buffer;
	OVERLAPPED recv_overlapped, send_overlapped;
	int recv_pendings_count, send_pendings_count;
	void *custom_data_ptr;
} OV_SOCKET;

IOCP_SERVER_INIT_INFO init_info = { 0 };

HANDLE				iocp_handle					= INVALID_HANDLE_VALUE;
volatile BOOL		terminate_requested			= FALSE;
SOCKET				listener					= INVALID_SOCKET;
SOCKET				acceptor					= INVALID_SOCKET;
char				*acceptor_buffer			= NULL;
DWORD				bytes_received_on_accept	= 0;
OVERLAPPED			listen_ov					= { 0 };
HANDLE				*threads					= NULL;

unsigned long __stdcall iocp_thread_worker(void *params);
int accept_pending();
void on_io(BOOL status, ULONG_PTR key, OVERLAPPED *ov_ptr, DWORD bytes_transferred);
OV_SOCKET *alloc_ov_socket(SOCKET s);
void release_ov_socket(OV_SOCKET **ov_socket);
void on_connect(OV_SOCKET *ov_socket);
void on_disconnect(OV_SOCKET *ov_socket);
void on_recv(OV_SOCKET *ov_socket);
void on_send(OV_SOCKET *ov_socket);

int iocp_server_init(IOCP_SERVER_INIT_INFO *info)
{
#ifdef _DEBUG
	printf(__FUNCTION__ "() threads_count: %d get_status_timeout: %d port: %d ip: %s buffer_size: %d on_connect_fptr: %p on_disconnect_fptr: %p on_recv_fptr: %p on_send_fptr: %p\n",
		info->threads_count,
		info->get_status_timeout,
		info->port,
		info->ip,
		info->buffers_size,
		info->on_connect_fptr,
		info->on_disconnect_fptr,
		info->on_recv_fptr,
		info->on_send_fptr
	);
#endif

	int error_level = 0;

	WSADATA wsa_data = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
	{
		error_level = IOCP_ERROR_WSA_STARTUP;
	}

	if (!error_level)
	{
		iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (!iocp_handle)
		{
			error_level = IOCP_ERROR_CREATE_IOCP;
		}
	}		

	if (!error_level)
	{
		if (!info->get_status_timeout)
		{
			info->get_status_timeout = IOCP_GET_STATUS_TIMEOUT_DEFAULT;
		}
	}	

	if (!error_level)
	{
		listener = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (listener == INVALID_SOCKET)
		{
			error_level = IOCP_ERROR_CREATE_LISTEN_SOCKET;
		}
	}	

	if (!error_level)
	{
		SOCKADDR_IN bind_addr = { 0 };
		bind_addr.sin_family = AF_INET;

		if (!info->port)
		{
			info->port = 25565;
		}

		bind_addr.sin_port = htons(info->port);

		if (!lstrlenA(info->ip))
		{
			StringCchCopyA(info->ip, IOCP_IP_ADDRESS_LENGTH, "0.0.0.0");
		}

		if (!inet_pton(AF_INET, info->ip, &bind_addr.sin_addr))
		{
			error_level = IOCP_ERROR_CONVERT_IP_ADDRESS;
		}

		if (!error_level)
		{
			if (bind(listener, (const SOCKADDR *)&bind_addr, sizeof(SOCKADDR)) == SOCKET_ERROR)
			{
				error_level = IOCP_ERROR_BIND;
			}
		}
	}	

	if (!error_level)
	{
		if (listen(listener, SOMAXCONN) == SOCKET_ERROR)
		{
			error_level = IOCP_ERROR_LISTEN;
		}
	}
	
	if (!error_level)
	{
		acceptor = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (acceptor == INVALID_SOCKET)
		{
			error_level = IOCP_ERROR_CREATE_ACCEPTOR_SOCKET;
		}
	}
	
	if (!info->buffers_size)
	{
		info->buffers_size = IOCP_BUFFERS_SIZE_DEFAULT;
	}

	if (!error_level)
	{
		acceptor_buffer = (char *)malloc(info->buffers_size);
		if (!acceptor_buffer)
		{
			error_level = IOCP_ERROR_ALLOC_ACCEPTOR_BUFFER;
		}		
	}

	if (!error_level)
	{
		error_level = accept_pending();
	}

	if (!error_level)
	{
		HANDLE new_iocp_handle = CreateIoCompletionPort((HANDLE)listener, iocp_handle, (ULONG_PTR)&listen_ov, 0);
		if (!new_iocp_handle)
		{
			error_level = IOCP_ERROR_CREATE_IOCP_WITH_LISTENER;
		}
		else
		{
			iocp_handle = new_iocp_handle;
		}
	}	

	if (!error_level)
	{
		if (!info->on_connect_fptr)
		{
			error_level = IOCP_ERROR_ON_CONNECT_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!info->on_disconnect_fptr)
		{
			error_level = IOCP_ERROR_ON_DISCONNECT_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!info->on_recv_fptr)
		{
			error_level = IOCP_ERROR_ON_RECV_FPTR_NOT_SET;
		}
	}

	if (!error_level)
	{
		if (!info->on_send_fptr)
		{
			error_level = IOCP_ERROR_ON_SEND_FPTR_NOT_SET;
		}
	}		

	if (!error_level)
	{
		if (!info->threads_count)
		{
			SYSTEM_INFO sys_info = { 0 };
			GetSystemInfo(&sys_info);
			info->threads_count = (int)sys_info.dwNumberOfProcessors;
		}

		threads = (HANDLE *)malloc((SIZE_T)info->threads_count * sizeof(HANDLE));
		if (!threads)
		{
			error_level = IOCP_ERROR_ON_SEND_FPTR_NOT_SET;
		}
	}	

#ifdef _DEBUG
	printf(__FUNCTION__ "() threads_count: %d get_status_timeout: %d port: %d ip: %s buffer_size: %d on_connect_fptr: %p on_disconnect_fptr: %p on_recv_fptr: %p on_send_fptr: %p\n",
		info->threads_count,
		info->get_status_timeout,
		info->port,
		info->ip,
		info->buffers_size,
		info->on_connect_fptr,
		info->on_disconnect_fptr,
		info->on_recv_fptr,
		info->on_send_fptr
	);
#endif

	if (!error_level)
	{
		for (SIZE_T i = 0; i < (SIZE_T)info->threads_count && !error_level; ++i)
		{
			threads[i] = CreateThread(NULL, 0, &iocp_thread_worker, NULL, 0, NULL);
			if (!threads[i])
			{
				error_level = IOCP_ERROR_CREATE_THREAD;
			}
		}
	}

	if (!error_level)
	{
		memcpy(&init_info, info, sizeof(IOCP_SERVER_INIT_INFO));

		terminate_requested = FALSE;
	}

	if (error_level)
	{
		iocp_server_terminate();
	}

	return error_level;
}

void iocp_server_terminate()
{
#ifdef _DEBUG
	printf(__FUNCTION__ "()\n");
#endif

	terminate_requested = TRUE;

	WaitForMultipleObjects((DWORD)init_info.threads_count, threads, TRUE, INFINITE);

	if (threads)
	{
		for (SIZE_T i = 0; i < (SIZE_T)init_info.threads_count; ++i)
		{
			CloseHandle(threads[i]);
		}

		free(threads);

		threads = NULL;
	}

	if (acceptor_buffer)
	{
		free(acceptor_buffer);
		acceptor_buffer = NULL;
	}

	if (acceptor != INVALID_SOCKET)
	{
		closesocket(acceptor);
		acceptor = INVALID_SOCKET;
	}
	
	if (listener != INVALID_SOCKET)
	{
		closesocket(listener);
		acceptor = INVALID_SOCKET;
	}	

	if (iocp_handle)
	{
		CloseHandle(iocp_handle);
		iocp_handle = INVALID_HANDLE_VALUE;
	}

	WSACleanup();
}

void iocp_connection_get_recv_data(void *handle, int *data_size, unsigned char **data_ptr)
{
	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	*data_ptr = (unsigned char *)ov_socket->raw_recv_buffer;
	*data_size = (int)ov_socket->recv_overlapped.InternalHigh;

#ifdef _DEBUG
	printf(__FUNCTION__"() handle: %p data_size: %d data_ptr: %p\n", handle, *data_size, *data_ptr);
#endif
}

int iocp_connection_recv(void *handle)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() handle: %p\n", handle);
#endif

	int error_level = 0;

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;

	DWORD flags		= 0;
	DWORD received	= 0;

	if (WSARecv(ov_socket->s, &ov_socket->recv_buffer, 1, &received, &flags, &ov_socket->recv_overlapped, NULL) == SOCKET_ERROR)
	{
		int res = WSAGetLastError();
		if (res != ERROR_IO_PENDING && res != WSAECONNRESET)
		{
			error_level = IOCP_ERROR_RECV;
		}
	}

	if (!error_level)
	{
		_InterlockedIncrement(&ov_socket->recv_pendings_count);
	}

	return error_level;
}

void iocp_connection_set_send_data(void *handle, int data_size, unsigned char *data)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() handle: %p data_size: %d data: %s\n", handle, data_size, (char *)data);
#endif

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	CopyMemory(ov_socket->raw_send_buffer, data, data_size);
	ov_socket->send_buffer.len = (int)data_size;
}

int iocp_connection_send(void *handle)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() handle: %p\n", handle);
#endif

	int error_level = 0;

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;

	DWORD flags		= 0;
	DWORD sended	= 0;

	if (WSASend(ov_socket->s, &ov_socket->send_buffer, 1, &sended, flags, &ov_socket->send_overlapped, NULL) == SOCKET_ERROR)
	{
		int res = WSAGetLastError();
		if (res != ERROR_IO_PENDING && res != WSAECONNRESET)
		{
			error_level = IOCP_ERROR_SEND;
		}
	}

	if (!error_level)
	{
		_InterlockedIncrement(&ov_socket->send_pendings_count);
	}

	return error_level;
}

void iocp_connection_disconnect(void *handle)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() handle: %p\n", handle);
#endif

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;

	SOCKET old_s = _InterlockedExchange64(&ov_socket->s, INVALID_SOCKET);
	if (old_s != INVALID_SOCKET)
	{
		shutdown(old_s, SD_BOTH);
		closesocket(old_s);

		on_disconnect(ov_socket);
	}
}

void iocp_connection_set_custom_data(void *handle, void *ptr)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() handle: %p ptr: %p\n", handle, ptr);
#endif

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	ov_socket->custom_data_ptr = ptr;
}

void *iocp_connection_get_custom_data(void *handle)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() handle: %p\n", handle);
#endif

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	return ov_socket->custom_data_ptr;
}

unsigned long __stdcall iocp_thread_worker(void *params)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() params: %p\n", params);
#endif

	DWORD		bytes_transferred	= 0;
	ULONG_PTR	key					= 0;
	OVERLAPPED	*overlapped			= NULL;	

	BOOL		status = FALSE;

	while (!terminate_requested)
	{
		status = GetQueuedCompletionStatus(iocp_handle, &bytes_transferred, &key, &overlapped, init_info.get_status_timeout);
		on_io(status, key, overlapped, bytes_transferred);

		bytes_transferred	= 0;
		key					= 0;
		overlapped			= NULL;
	}

	return 0;
}

int accept_pending()
{
#ifdef _DEBUG
	printf(__FUNCTION__ "()\n");
#endif

	int error_level = 0;

	if (!AcceptEx(
		listener,
		acceptor,
		acceptor_buffer,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&bytes_received_on_accept,
		&listen_ov))
	{
		int res = WSAGetLastError();
		if (res != ERROR_IO_PENDING)
		{
			error_level = IOCP_ERROR_ACCEPT_PENDING;
		}
	}

	return error_level;
}

void on_io(BOOL status, ULONG_PTR key, OVERLAPPED *ov_ptr, DWORD bytes_transferred)
{
#ifdef _DEBUG
	//printf(__FUNCTION__" status: %d key: %llu ov_ptr: %p bytes_transferred: %d\n", status, key, ov_ptr, bytes_transferred);
#endif

	if (status)
	{
		if (key)
		{
			if (ov_ptr)
			{
				if (ov_ptr == &listen_ov)
				{
					OV_SOCKET *ov_socket = alloc_ov_socket(acceptor);
					if (!ov_socket)
					{
						return;
					}

					acceptor = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
					if (acceptor == INVALID_SOCKET)
					{
						return;
					}

					if (accept_pending() != 0)
					{
						return;
					}

					HANDLE new_iocp_handle = CreateIoCompletionPort((HANDLE)ov_socket->s, iocp_handle, (ULONG_PTR)ov_socket, 0);
					if (!new_iocp_handle)
					{
						return;
					}

					iocp_handle = new_iocp_handle;

					on_connect(ov_socket);
					
					if (!iocp_connection_recv(ov_socket))
					{
						return;
					}
					
				}
				else
				{
					OV_SOCKET *ov_socket = (OV_SOCKET *)key;

					if (bytes_transferred)
					{
						if (ov_ptr == &ov_socket->recv_overlapped)
						{
							on_recv(ov_socket);

							_InterlockedDecrement(&ov_socket->recv_pendings_count);

							ZeroMemory(ov_socket->raw_recv_buffer, ov_socket->recv_buffer.len);

							if (!iocp_connection_recv(ov_socket))
							{
								return;
							}
						}
						else if (ov_ptr == &ov_socket->send_overlapped)
						{
							on_send(ov_socket);

							_InterlockedDecrement(&ov_socket->send_pendings_count);

							ZeroMemory(ov_socket->raw_send_buffer, ov_socket->send_buffer.len);
							ov_socket->send_buffer.len = 0;
						}
					}
					else
					{
						if (_InterlockedExchange(&ov_socket->recv_pendings_count, ov_socket->recv_pendings_count))
						{
							_InterlockedDecrement(&ov_socket->recv_pendings_count);
						}

						if (_InterlockedExchange(&ov_socket->send_pendings_count, ov_socket->send_pendings_count))
						{
							_InterlockedDecrement(&ov_socket->send_pendings_count);
						}

						if (!_InterlockedExchange(&ov_socket->recv_pendings_count, ov_socket->recv_pendings_count) &&
							!_InterlockedExchange(&ov_socket->send_pendings_count, ov_socket->send_pendings_count))
						{
							SOCKET old_s = _InterlockedExchange64(&ov_socket->s, INVALID_SOCKET);
							if (old_s != INVALID_SOCKET)
							{
								shutdown(old_s, SD_BOTH);
								closesocket(old_s);

								on_disconnect(ov_socket);								
							}

							release_ov_socket(&ov_socket);
						}
					}
				}
			}
		}
	}
	else
	{
		if (key)
		{
			OV_SOCKET *ov_socket = (OV_SOCKET *)key;			

			if (_InterlockedExchange(&ov_socket->recv_pendings_count, ov_socket->recv_pendings_count))
			{
				_InterlockedDecrement(&ov_socket->recv_pendings_count);
			}

			if (_InterlockedExchange(&ov_socket->send_pendings_count, ov_socket->send_pendings_count))
			{
				_InterlockedDecrement(&ov_socket->send_pendings_count);
			}

			if (!_InterlockedExchange(&ov_socket->recv_pendings_count, ov_socket->recv_pendings_count) &&
				!_InterlockedExchange(&ov_socket->send_pendings_count, ov_socket->send_pendings_count))
			{
				SOCKET old_s = _InterlockedExchange64(&ov_socket->s, INVALID_SOCKET);
				if (old_s != INVALID_SOCKET)
				{
					shutdown(old_s, SD_BOTH);
					closesocket(old_s);

					on_disconnect(ov_socket);					
				}

				release_ov_socket(&ov_socket);
			}
		}
	}
}

OV_SOCKET *alloc_ov_socket(SOCKET s)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() s: %llu\n", s);
#endif

	OV_SOCKET *ov_socket = (OV_SOCKET *)malloc(sizeof(OV_SOCKET));
	if (ov_socket)
	{
		ZeroMemory(ov_socket, sizeof(OV_SOCKET));
		ov_socket->s = s;

		ov_socket->raw_recv_buffer = (char *)malloc((SIZE_T)init_info.buffers_size);
		ZeroMemory(ov_socket->raw_recv_buffer, (SIZE_T)init_info.buffers_size);
		ov_socket->recv_buffer.buf = ov_socket->raw_recv_buffer;
		ov_socket->recv_buffer.len = (ULONG)init_info.buffers_size;

		ov_socket->raw_send_buffer = (char *)malloc((SIZE_T)init_info.buffers_size);
		ZeroMemory(ov_socket->raw_send_buffer, (SIZE_T)init_info.buffers_size);
		ov_socket->send_buffer.buf = ov_socket->raw_send_buffer;
		ov_socket->send_buffer.len = 0;
	}	
	return ov_socket;
}

void release_ov_socket(OV_SOCKET **ov_socket)
{
#ifdef _DEBUG
	printf(__FUNCTION__"() ov_socket: %p\n", *ov_socket);
#endif
	
	free((*ov_socket)->raw_send_buffer);
	(*ov_socket)->raw_send_buffer = NULL;
	
	free((*ov_socket)->raw_recv_buffer);
	(*ov_socket)->raw_recv_buffer = NULL;	
	
	free(*ov_socket);
	*ov_socket = NULL;
}

void on_connect(OV_SOCKET *ov_socket)
{
	init_info.on_connect_fptr(ov_socket);
}

void on_disconnect(OV_SOCKET *ov_socket)
{
	init_info.on_disconnect_fptr(ov_socket);
}

void on_recv(OV_SOCKET *ov_socket)
{
	init_info.on_recv_fptr(ov_socket);
}

void on_send(OV_SOCKET *ov_socket)
{
	init_info.on_send_fptr(ov_socket);
}
