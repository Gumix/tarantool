/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2010-2024, Tarantool AUTHORS, please see AUTHORS file.
 */

#pragma once

#include "arrow/abi.h"

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * Arrow record batch is a two-dimensional dataset in the columnar format.
 */
struct arrow_record_batch {
	/** Describes the type and metadata. */
	struct ArrowSchema schema;
	/** Describes the data. */
	struct ArrowArray array;
};

/**
 * TODO
 */
struct arrow_record_batch *
arrow_record_batch_unpack(const char **data, uint32_t len,
			  struct arrow_record_batch *arrow);

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */
