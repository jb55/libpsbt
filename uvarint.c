
#define _DEFAULT_SOURCE

#include <limits.h>
#include <endian.h>
#include <string.h>

#include "uvarint.h"

u32 uvarint_length(u64 data) {
	u32 len = 1;
	while (data >>= 7) {
		++len;
	}
	return len;
}

void uvarint_write(u8 *dest, u64 size)
{
	while (1) {
		*dest = size & 0x7f;
		size >>= 7;
		if (size) *dest |= 0x80;
		else return;
	}
}

u32 uvarint_read(u64* dst, u8 *data, const u8 *limit) {
	u8 *p = data;
	*dst = 0;
	u32 ctr = 0;
	while (p < limit) {
		++ctr;
		*dst <<= 7;
		*dst |= *p & 0x7f;
		if (*dst & 0x80) ++p;
		else return ctr;
	}
	return 0;
}
