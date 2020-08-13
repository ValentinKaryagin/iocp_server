#include "iocp_thread_worker.h"

#include "iocp_server_ctx.h"

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
