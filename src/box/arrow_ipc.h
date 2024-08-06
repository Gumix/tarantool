/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2024, Tarantool AUTHORS, please see AUTHORS file.
 */
#pragma once

#include "arrow/abi.h"

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

struct arrow_ipc_buffer;

/** Allocates and initializes an Arrow IPC buffer, can not fail. */
struct arrow_ipc_buffer *
arrow_ipc_buffer_new(void);

/** Destroys and deallocates an Arrow IPC buffer. */
void
arrow_ipc_buffer_delete(struct arrow_ipc_buffer *ipc_buffer);

/**
 * Encodes `array' and `schema' into Arrow IPC format. The memory is allocated
 * by the `ipc_buffer', it must be initialized by `arrow_ipc_buffer_new()'.
 * Pointers to the encoded data are returned via `ret_data' and `ret_data_end'.
 * Returns 0 on success, -1 on failure (diag is set).
 */
int
arrow_ipc_encode(struct ArrowArray *array, struct ArrowSchema *schema,
		 struct arrow_ipc_buffer *ipc_buffer,
		 const char **ret_data, const char **ret_data_end);

/**
 * Decodes `array' and `schema' from the `data' in Arrow IPC format.
 * Returns 0 on success, -1 on failure (diag is set).
 */
int
arrow_ipc_decode(struct ArrowArray *array, struct ArrowSchema *schema,
		 const char *data, const char *data_end);

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */
