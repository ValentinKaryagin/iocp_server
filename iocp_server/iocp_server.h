#pragma once

#define IOCP_IP_ADDRESS_LENGTH					16

#define IOCP_BUFFERS_SIZE_DEFAULT				8196
#define IOCP_GET_STATUS_TIMEOUT_DEFAULT			1

#define IOCP_ERROR_WSA_STARTUP					1
#define IOCP_ERROR_CREATE_IOCP					2
#define IOCP_ERROR_CREATE_LISTEN_SOCKET			3
#define IOCP_ERROR_CONVERT_IP_ADDRESS			4
#define IOCP_ERROR_BIND							5
#define IOCP_ERROR_LISTEN						6
#define IOCP_ERROR_CREATE_ACCEPTOR_SOCKET		7
#define IOCP_ERROR_ALLOC_ACCEPTOR_BUFFER		8
#define IOCP_ERROR_ACCEPT_PENDING				9
#define IOCP_ERROR_CREATE_IOCP_WITH_LISTENER	10
#define IOCP_ERROR_ON_CONNECT_FPTR_NOT_SET		11
#define IOCP_ERROR_ON_DISCONNECT_FPTR_NOT_SET	12
#define IOCP_ERROR_ON_RECV_FPTR_NOT_SET			13
#define IOCP_ERROR_ON_SEND_FPTR_NOT_SET			14
#define IOCP_ERROR_ALLOC_THREADS_CONTAINER		15
#define IOCP_ERROR_CREATE_THREAD				16
#define IOCP_ERROR_RECV							17
#define IOCP_ERROR_SEND							18

typedef void (on_connect_func)(void *);
typedef void (on_disconnect_func)(void *);
typedef void (on_recv_func)(void *);
typedef void (on_send_func)(void *);

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _IOCP_SERVER_INIT_INFO
	{		
		int					get_status_timeout;
		unsigned short		port;
		char				ip[IOCP_IP_ADDRESS_LENGTH];
		int					buffers_size;
		on_connect_func		*on_connect_fptr;
		on_disconnect_func	*on_disconnect_fptr;
		on_recv_func		*on_recv_fptr;
		on_send_func		*on_send_fptr;
		int					threads_count;
	} IOCP_SERVER_INIT_INFO;

	int iocp_server_init(IOCP_SERVER_INIT_INFO *info);
	void iocp_server_terminate();

	void iocp_connection_get_recv_data(void *handle, int *data_size, unsigned char **data_ptr);
	int iocp_connection_recv(void *handle);

	void iocp_connection_set_send_data(void *handle, int data_size, unsigned char *data);
	int iocp_connection_send(void *handle);

	void iocp_connection_disconnect(void *handle);

	void iocp_connection_set_custom_data(void *handle, void *ptr);
	void *iocp_connection_get_custom_data(void *handle);
#ifdef __cplusplus
}
#endif
