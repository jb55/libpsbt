
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <assert.h>
#include "compactsize.h"

char *psbt_errmsg = NULL;

#define ASSERT_SPACE(s) \
	if (tx->write_pos+(s) > tx->data + tx->data_capacity) { \
		psbt_errmsg = "psbt_write_kv: write out of bounds"; \
		return PSBT_OUT_OF_BOUNDS_WRITE; \
	}


size_t psbt_size(struct psbt *tx) {
	return tx->write_pos - tx->data -1;
}


static enum psbt_result
psbt_begin(struct psbt *tx) {
	u32 magic = htobe32(PSBT_MAGIC);

	ASSERT_SPACE(sizeof(magic));
	memcpy(tx->write_pos, &magic, sizeof(magic));
	tx->write_pos += sizeof(magic);

	ASSERT_SPACE(1);
	*tx->write_pos = 0xff;
	tx->write_pos++;

	return PSBT_OK;
}


enum psbt_result
psbt_init(struct psbt *tx, unsigned char *dest, size_t dest_size) {
	tx->write_pos = dest;
	tx->data = dest;
	tx->data_capacity = dest_size;
	return psbt_begin(tx);
}

/* enum psbt_result */
/* psbt_end_transaction(char *dest, size_t dest_len) */
/* { */
/* 	ASSERT_SPACE(1); */
/* 	*dest =  */
/* } */

enum psbt_result
psbt_write_record(struct psbt *tx, struct psbt_record *rec) {
	u32 size;

	// write key length
	size = compactsize_length(rec->key_size);
	assert(size == 1);
	ASSERT_SPACE(size);
	compactsize_write((u8*)tx->write_pos, rec->key_size);
	tx->write_pos += size;

	// write key
	ASSERT_SPACE(rec->key_size);
	memcpy(tx->write_pos, rec->key, rec->key_size);
	tx->write_pos += rec->key_size;

	// write value length
	size = compactsize_length(rec->val_size);
	ASSERT_SPACE(size);
	compactsize_write((u8*)tx->write_pos, rec->val_size);
	tx->write_pos += size;

	// write value
	ASSERT_SPACE(rec->val_size);
	memcpy(tx->write_pos, rec->val, rec->val_size);
	tx->write_pos += rec->val_size;

	return PSBT_OK;
}
