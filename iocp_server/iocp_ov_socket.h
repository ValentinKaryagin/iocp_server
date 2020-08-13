#pragma once

#include "iocp_server_ctx.h"

typedef struct _OV_SOCKET
{
	SOCKET s;
	char *raw_recv_buffer, *raw_send_buffer;
	WSABUF recv_buffer, send_buffer;
	OVERLAPPED recv_overlapped, send_overlapped;
	int recv_pendings_count, send_pendings_count;
	void *custom_data_ptr;
} OV_SOCKET;

#ifdef __cplusplus
extern "C"
{
#endif
	OV_SOCKET *alloc_ov_socket(SOCKET s, IOCP_SERVER_CTX *ctx);
	void release_ov_socket(OV_SOCKET **ov_socket);
	void on_connect(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx);
	void on_disconnect(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx);
	void on_recv(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx);
	void on_send(OV_SOCKET *ov_socket, IOCP_SERVER_CTX *ctx);
#ifdef __cplusplus
}
#endif
