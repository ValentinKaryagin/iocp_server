#include "iocp_thread_worker.h"

#include "iocp_ov_socket.h"
#include "iocp_connection.h"

void on_io(BOOL status, ULONG_PTR key, OVERLAPPED *ov_ptr, DWORD bytes_transferred, IOCP_SERVER_CTX *ctx);

unsigned long __stdcall iocp_thread_worker(void *params)
{
	IOCP_SERVER_CTX *ctx = (IOCP_SERVER_CTX *)params;

	DWORD		bytes_transferred = 0;
	ULONG_PTR	key = 0;
	OVERLAPPED *overlapped = NULL;

	BOOL		status = FALSE;

	while (!ctx->terminate_requested)
	{
		status = GetQueuedCompletionStatus(ctx->iocp_handle, &bytes_transferred, &key, &overlapped, ctx->settings.get_status_timeout);
		on_io(status, key, overlapped, bytes_transferred, ctx);

		bytes_transferred = 0;
		key = 0;
		overlapped = NULL;
	}

	return 0;
}

void on_io(BOOL status, ULONG_PTR key, OVERLAPPED *ov_ptr, DWORD bytes_transferred, IOCP_SERVER_CTX *ctx)
{
	if (status)
	{
		if (key)
		{
			if (ov_ptr)
			{
				if (ov_ptr == &ctx->listen_ov)
				{
					HANDLE new_iocp_handle = INVALID_HANDLE_VALUE;
					OV_SOCKET *ov_socket = alloc_ov_socket(ctx->acceptor, ctx);
					if (!ov_socket)
					{
						return;
					}

					ctx->acceptor = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
					if (ctx->acceptor == INVALID_SOCKET)
					{
						return;
					}

					if (accept_pending(ctx) != 0)
					{
						return;
					}

					new_iocp_handle = CreateIoCompletionPort((HANDLE)ov_socket->s, ctx->iocp_handle, (ULONG_PTR)ov_socket, 0);
					if (!new_iocp_handle)
					{
						return;
					}

					ctx->iocp_handle = new_iocp_handle;

					on_connect(ov_socket, ctx);

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
						if (_InterlockedExchange64(&ov_socket->s, ov_socket->s) != INVALID_SOCKET)
						{
							if (ov_ptr == &ov_socket->recv_overlapped)
							{
								_InterlockedDecrement(&ov_socket->recv_pendings_count);
								on_recv(ov_socket, ctx);

								if (!iocp_connection_recv(ov_socket))
								{
									return;
								}
							}
							else if (ov_ptr == &ov_socket->send_overlapped)
							{
								_InterlockedDecrement(&ov_socket->send_pendings_count);
								on_send(ov_socket, ctx);
							}
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

								on_disconnect(ov_socket, ctx);

								release_ov_socket(&ov_socket);
							}
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

					on_disconnect(ov_socket, ctx);

					release_ov_socket(&ov_socket);
				}
			}
		}
	}
}
