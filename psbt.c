
#include <stdio.h>
#include <string.h>
#include <endian.h>
#include "compactsize.h"

char *psbt_errmsg = NULL;

#define ASSERT_SPACE(s)	      \
	if (tx->write_pos+(s) > tx->data + tx->data_size) {				    \
		psbt_errmsg = "psbt_write_kv: write out of bounds"; \
		return PSBT_OUT_OF_BOUNDS_WRITE; \
	}


enum psbt_result
psbt_begin_transaction(struct psbt_transaction *tx) {
	u32 magic = htobe32(PSBT_MAGIC);
	ASSERT_SPACE(sizeof(magic));
	memcpy(tx->write_pos, &magic, sizeof(magic));
	p += sizeof(magic);
	*tx->write_pos++ = 0xff;
	return PSBT_OK;
}


enum psbt_result
psbt_init_transaction(struct psbt_transaction *tx, char *dest, char dest_size) {
	tx->write_pos = dest;
	tx->data = dest;
	tx->data_size = dest_size;
	return psbt_begin_transaction(tx);
}
	

enum psbt_result
psbt_end_transaction(char *dest, size_t dest_len)
{
	ASSERT_SPACE(1);
	*dest = 
}

enum psbt_result
psbt_write_kv(char *dest, size_t dest_len,
	      const char *key, unsigned int key_len,
	      const char *value, unsigned int val_len,
	      size_t *written)
{
	char *p = dest;
	u32 size;

	// write key lengthg
	size = compactsize_length(key_len);
	ASSERT_SPACE(size);
	compactsize_write((u8*)p, key_len);
	p += size;

	// write key
	ASSERT_SPACE(key_len);
	memcpy(p, key, key_len);
	p += key_len;

	// write value length
	size = compactsize_length(val_len);
	ASSERT_SPACE(size);
	compactsize_write((u8*)p, val_len);
	p += size;

	// write value
	ASSERT_SPACE(val_len);
	memcpy(p, value, val_len);
	p += val_len;

	*written = p - dest;

	return PSBT_OK;
}
