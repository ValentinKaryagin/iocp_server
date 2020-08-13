#include "iocp_connection.h"

#include "iocp_ov_socket.h"

#include "iocp_error_codes.h"

void iocp_connection_get_recv_data(void *handle, int *data_size, unsigned char **data_ptr)
{
	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	*data_ptr = (unsigned char *)ov_socket->raw_recv_buffer;
	*data_size = (int)ov_socket->recv_overlapped.InternalHigh;
}

int iocp_connection_recv(void *handle)
{
	int error_level = 0;

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;

	DWORD flags = 0;
	DWORD received = 0;

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
	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	CopyMemory(ov_socket->raw_send_buffer, data, data_size);
	ov_socket->send_buffer.len = (int)data_size;
}

int iocp_connection_send(void *handle)
{
	int error_level = 0;

	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;

	DWORD flags = 0;
	DWORD sended = 0;

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
	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;

	SOCKET old_s = _InterlockedExchange64(&ov_socket->s, INVALID_SOCKET);
	if (old_s != INVALID_SOCKET)
	{
		shutdown(old_s, SD_BOTH);
		closesocket(old_s);
	}
}

void iocp_connection_set_custom_data(void *handle, void *ptr)
{
	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	ov_socket->custom_data_ptr = ptr;
}

void *iocp_connection_get_custom_data(void *handle)
{
	OV_SOCKET *ov_socket = (OV_SOCKET *)handle;
	return ov_socket->custom_data_ptr;
}
