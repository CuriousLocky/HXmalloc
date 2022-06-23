#pragma once

#include <stddef.h>
#include <stdint.h>

void *hxmalloc(size_t size);
void hxfree(void *ptr);
void *hxrealloc(void *ptr, size_t newSize);
