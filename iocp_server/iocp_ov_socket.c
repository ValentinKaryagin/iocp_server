#include "iocp_ov_socket.h"

#include "iocp_error_codes.h"

int accept_pending(IOCP_SERVER_CTX *ctx)
{
	int error_level = 0;

	if (!AcceptEx(
		ctx->listener,
		ctx->acceptor,
		ctx->acceptor_buffer,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&ctx->bytes_received_on_accept,
		&ctx->listen_ov))
	{
		int res = WSAGetLastError();
		if (res != ERROR_IO_PENDING)
		{
			error_level = IOCP_ERROR_ACCEPT_PENDING;
		}
	}

	return error_level;
}

OV_SOCKET *alloc_ov_socket(SOCKET s, IOCP_SERVER_CTX *ctx)
{
	OV_SOCKET *ov_socket = (OV_SOCKET *)malloc(sizeof(OV_SOCKET));
	if (ov_socket)
	{
		ZeroMemory(ov_socket, sizeof(OV_SOCKET));
		ov_socket->s = s;

		ov_socket->raw_recv_buffer = (char *)malloc((SIZE_T)ctx->settings.buffers_size);
		ZeroMemory(ov_socket->raw_recv_buffer, (SIZE_T)ctx->settings.buffers_size);
		ov_socket->recv_buffer.buf = ov_socket->raw_recv_buffer;
		ov_socket->recv_buffer.len = (ULONG)ctx->settings.buffers_size;

		ov_socket->raw_send_buffer = (char *)malloc((SIZE_T)ctx->settings.buffers_size);
		ZeroMemory(ov_socket->raw_send_buffer, (SIZE_T)ctx->settings.buffers_size);
		ov_socket->send_buffer.buf = ov_socket->raw_send_buffer;
		ov_socket->send_buffer.len = 0;
	}
	return ov_socket;
}

void release_ov_socket(OV_SOCKET **ov_socket)
{
	free((*ov_socket)->raw_send_buffer);
	(*ov_socket)->raw_send_buffer = NULL;

	free((*ov_socket)->raw_recv_buffer);
	(*ov_socket)->raw_recv_buffer = NULL;

	free(*ov_socket);
	*ov_socket = NULL;
}

void on_connect(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx)
{
	ctx->event_listener.on_connected_fptr(ov_socket);
}

void on_disconnect(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx)
{
	ctx->event_listener.on_disconnected_fptr(ov_socket);
}

void on_recv(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx)
{
	ctx->event_listener.on_received_fptr(ov_socket);
}

void on_send(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx)
{
	ctx->event_listener.on_sended_fptr(ov_socket);
}
