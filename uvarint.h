
#ifndef PSBT_UVARINT_H
#define PSBT_UVARINT_H

#include "psbt.h"
#include "common.h"

u32 uvarint_read(u64 *dst, u8 *data, const u8 *max);
u32 uvarint_length(u64 data);
void uvarint_write(u8 *dest, u64 size);

#endif /* PSBT_UVARINT_H */
