/**
 * TODO
 */

#include "core/arrow.h"

#include "stddef.h"
#include "assert.h"
#include "box/arrow_ipc.h"

struct arrow_record_batch *
arrow_unpack(const char **data, uint32_t len, struct arrow_record_batch *arrow)
{
	/*
	 * MsgPack extensions have length greater or equal than 1 by
	 * specification.
	 */
	assert(len > 0);

	if (arrow_ipc_decode(&arrow->array, &arrow->schema, *data,
			     *data + len) != 0)
		return NULL;
	assert(arrow->array.release != NULL);
	assert(arrow->schema.release != NULL);

	*data += len;
	return arrow;
}

int
mp_validate_arrow(const char *data, uint32_t len)
{
	struct arrow_record_batch arrow;
	struct arrow_record_batch *rc = arrow_unpack(&data, len, &arrow);
	arrow.array.release(&arrow.array);
	arrow.schema.release(&arrow.schema);
	return rc == NULL;
}
