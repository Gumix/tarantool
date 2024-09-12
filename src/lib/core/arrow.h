/**
 * TODO
 */
#pragma once

#include "arrow/abi.h"

struct arrow_record_batch {
	struct ArrowArray array;
	struct ArrowSchema schema;
};
