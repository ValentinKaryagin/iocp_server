#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
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
