/**
 * TODO
 */
#pragma once

#include <stdint.h>

struct arrow_record_batch;

struct arrow_record_batch *
arrow_unpack(const char **data, uint32_t len, struct arrow_record_batch *arrow);

int
mp_validate_arrow(const char *data, uint32_t len);
