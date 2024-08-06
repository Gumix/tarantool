/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2024, Tarantool AUTHORS, please see AUTHORS file.
 */
#include "arrow_ipc.h"

#include "diag.h"
#include "error.h"
#include "tt_static.h"
#include "nanoarrow/nanoarrow_ipc.h"

/** Arrow IPC buffer, used for storing the encoded data. */
struct arrow_ipc_buffer {
	/** The implementation. */
	struct ArrowBuffer base;
};

struct arrow_ipc_buffer *
arrow_ipc_buffer_new(void)
{
	struct arrow_ipc_buffer *ipc_buffer = xmalloc(sizeof(*ipc_buffer));
	ArrowBufferInit(&ipc_buffer->base);
	return ipc_buffer;
}

void
arrow_ipc_buffer_delete(struct arrow_ipc_buffer *ipc_buffer)
{
	struct ArrowBuffer *buffer = &ipc_buffer->base;
	ArrowBufferReset(buffer);
	TRASH(buffer);
	free(buffer);
}

int
arrow_ipc_encode(struct ArrowArray *array, struct ArrowSchema *schema,
		 struct arrow_ipc_buffer *ipc_buffer,
		 const char **ret_data, const char **ret_data_end)
{
	ArrowErrorCode rc;
	struct ArrowError error;
	struct ArrowBuffer *buffer = &ipc_buffer->base;

	struct ArrowArrayView array_view;
	rc = ArrowArrayViewInitFromSchema(&array_view, schema, &error);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_ENCODE,
			 tt_sprintf("ArrowArrayViewInitFromSchema: %s",
				    error.message));
		return -1;
	}

	/* Set buffer sizes and data pointers from an array. */
	rc = ArrowArrayViewSetArray(&array_view, array, &error);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_ENCODE,
			 tt_sprintf("ArrowArrayViewSetArray: %s",
				    error.message));
		goto error1;
	}

	/*
	 * All bytes written to the stream will be appended to the buffer.
	 * The stream takes ownership of the buffer.
	 */
	struct ArrowIpcOutputStream stream;
	rc = ArrowIpcOutputStreamInitBuffer(&stream, buffer);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_ENCODE,
			 "ArrowIpcOutputStreamInitBuffer");
		goto error1;
	}

	/*
	 * A stream writer which encodes schema and array into an IPC byte
	 * stream. The writer takes ownership of the output byte stream.
	 */
	struct ArrowIpcWriter writer;
	rc = ArrowIpcWriterInit(&writer, &stream);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_ENCODE,
			 "ArrowIpcWriterInit");
		stream.release(&stream);
		goto error1;
	}

	rc = ArrowIpcWriterWriteSchema(&writer, schema, &error);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_ENCODE,
			 tt_sprintf("ArrowIpcWriterWriteSchema: %s",
				    error.message));
		goto error2;
	}

	rc = ArrowIpcWriterWriteArrayView(&writer, &array_view, &error);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_ENCODE,
			 tt_sprintf("ArrowIpcWriterWriteArrayView: %s",
				    error.message));
		goto error2;
	}

	*ret_data = (const char *)buffer->data;
	*ret_data_end = (const char *)buffer->data + buffer->size_bytes;

	ArrowIpcWriterReset(&writer);
	ArrowArrayViewReset(&array_view);
	return 0;
error2:
	ArrowIpcWriterReset(&writer);
error1:
	ArrowArrayViewReset(&array_view);
	return -1;
}

int
arrow_ipc_decode(struct ArrowArray *array, struct ArrowSchema *schema,
		 const char *data, const char *data_end)
{
	ssize_t size = data_end - data;
	if (size <= 0) {
		diag_set(ClientError, ER_ARROW_IPC_DECODE,
			 "Unexpected data size");
		return -1;
	}

	ArrowErrorCode rc;
	struct ArrowError error;
	struct ArrowBuffer buffer;
	ArrowBufferInit(&buffer);

	rc = ArrowBufferAppend(&buffer, data, size);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_DECODE, "ArrowBufferAppend");
		ArrowBufferReset(&buffer);
		return -1;
	}

	/*
	 * Create an input stream from a buffer.
	 * The stream takes ownership of the buffer and reads bytes from it.
	 */
	struct ArrowIpcInputStream input_stream;
	rc = ArrowIpcInputStreamInitBuffer(&input_stream, &buffer);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_DECODE,
			 "ArrowIpcInputStreamInitBuffer");
		ArrowBufferReset(&buffer);
		return -1;
	}

	/*
	 * Initialize an array stream from an input stream of bytes.
	 * The array_stream takes ownership of input_stream.
	 */
	struct ArrowArrayStream array_stream;
	rc = ArrowIpcArrayStreamReaderInit(&array_stream, &input_stream, NULL);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_DECODE,
			 "ArrowIpcArrayStreamReaderInit");
		input_stream.release(&input_stream);
		return -1;
	}

	rc = ArrowArrayStreamGetSchema(&array_stream, schema, &error);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_DECODE,
			 tt_sprintf("ArrowArrayStreamGetSchema: %s",
				    error.message));
		goto error;
	}

	rc = ArrowArrayStreamGetNext(&array_stream, array, &error);
	if (rc != NANOARROW_OK) {
		diag_set(ClientError, ER_ARROW_IPC_DECODE,
			 tt_sprintf("ArrowArrayStreamGetNext: %s",
				    error.message));
		goto error;
	}

	ArrowArrayStreamRelease(&array_stream);
	return 0;
error:
	ArrowArrayStreamRelease(&array_stream);
	return -1;
}
