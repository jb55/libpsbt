
#ifndef PSBT_COMPACTSIZE_H
#define PSBT_COMPACTSIZE_H

#include "psbt.h"
#include "common.h"

u64 compactsize_read(u8 *data, enum psbt_result *err);
u32 compactsize_length(u64 data);
void compactsize_write(u8 *dest, u64 size);
u32 compactsize_peek_length(u8 chsize);

#endif /* PSBT_COMPACTSIZE_H */
