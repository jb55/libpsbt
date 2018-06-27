
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <assert.h>
#include "compactsize.h"
#include "uvarint.h"

char *psbt_errmsg = NULL;

#define ASSERT_SPACE(s) \
	if (tx->write_pos+(s) > tx->data + tx->data_capacity) { \
		psbt_errmsg = "write out of bounds"; \
		return PSBT_OUT_OF_BOUNDS_WRITE; \
	}

size_t psbt_size(struct psbt *tx) {
	return tx->write_pos - tx->data;
}


static enum psbt_result
psbt_write_header(struct psbt *tx) {
	u32 magic = htobe32(PSBT_MAGIC);

	ASSERT_SPACE(sizeof(magic));
	memcpy(tx->write_pos, &magic, sizeof(magic));
	tx->write_pos += sizeof(magic);

	ASSERT_SPACE(1);
	*tx->write_pos = 0xff;
	tx->write_pos++;

	tx->state = PSBT_ST_HEADER;

	return PSBT_OK;
}


enum psbt_result
psbt_init(struct psbt *tx, unsigned char *dest, size_t dest_size) {
	tx->write_pos = dest;
	tx->data = dest;
	tx->data_capacity = dest_size;
	tx->state = PSBT_ST_INIT;
	return PSBT_OK;
}

/* enum psbt_result */
/* psbt_end_transaction(char *dest, size_t dest_len) */
/* { */
/* 	ASSERT_SPACE(1); */
/* 	*dest =  */
/* } */


enum psbt_result
psbt_close_records(struct psbt *tx) {
	ASSERT_SPACE(1);
	*tx->write_pos = '\0';
	tx->write_pos++;
	return PSBT_OK;
}


enum psbt_result
psbt_finalize(struct psbt *tx) {
	enum psbt_result res;

	/*if (tx->state != PSBT_ST_INPUTS_NEW && tx->state != PSBT_ST_INPUTS) {
		psbt_errmsg = "psbt_finalize: no input records found";
		return PSBT_INVALID_STATE;
	}

	res = psbt_close_records(tx);
	if (res != PSBT_OK)
		return res;*/

	tx->state = PSBT_ST_FINALIZED;

	return PSBT_OK;
}

static enum psbt_result
psbt_write_record(struct psbt *tx, struct psbt_record *rec) {
	u32 size;
	u32 type_entry = (rec->type << 3) + 2;

	// write type length
	size = uvarint_length(type_entry);
	ASSERT_SPACE(size);
	uvarint_write((u8*)tx->write_pos, type_entry);
	tx->write_pos += size;

	// write value length
	size = uvarint_length(rec->val_size);
	ASSERT_SPACE(size);
	uvarint_write((u8*)tx->write_pos, rec->val_size);
	tx->write_pos += size;

	// write value
	ASSERT_SPACE(rec->val_size);
	memcpy(tx->write_pos, rec->val, rec->val_size);
	tx->write_pos += rec->val_size;

	return PSBT_OK;
}

static enum psbt_result
psbt_read_header(struct psbt *tx) {
	ASSERT_SPACE(4);

	u32 magic = be32toh(*((u32*)tx->write_pos));

	tx->write_pos += 4;

	if (magic != PSBT_MAGIC) {
		psbt_errmsg = "psbt_read: invalid magic header";
		return PSBT_READ_ERROR;
	}

	if (*tx->write_pos++ != 0xff) {
		psbt_errmsg = "psbt_read: no 0xff found after magic";
		return PSBT_READ_ERROR;
	}

	tx->state = PSBT_ST_GLOBAL;

	return PSBT_OK;
}


static enum psbt_result
psbt_read_record(struct psbt *tx, size_t src_size, struct psbt_record *rec)
{
	enum psbt_result res;
	u64 size;
	u32 size_len;

	size_len = compactsize_peek_length(*tx->write_pos);
	ASSERT_SPACE(size_len);
	size = compactsize_read(tx->write_pos, &res);

	tx->write_pos += size_len;

	if (res != PSBT_OK)
		return res;

	if (tx->write_pos + size > tx->data + src_size) {
		psbt_errmsg = "psbt_read: record key size too large";
		return PSBT_READ_ERROR;
	}

	ASSERT_SPACE(1 + size);

	rec->key_size = size;
	rec->key = tx->write_pos + 1;
	rec->is_global = tx->state == PSBT_ST_GLOBAL;

	rec->type = *tx->write_pos;

	tx->write_pos += 1 + size;

	size_len = compactsize_peek_length(*tx->write_pos);
	ASSERT_SPACE(size_len);
	size = compactsize_read(tx->write_pos, &res);

	if (res != PSBT_OK)
		return res;

	tx->write_pos += size_len;

	if (tx->write_pos + size > tx->data + src_size) {
		psbt_errmsg = "psbt_read: record value size too large";
		return PSBT_READ_ERROR;
	}

	rec->val_size = size;
	rec->val = tx->write_pos;

	ASSERT_SPACE(size);
	tx->write_pos += size;

	return PSBT_OK;
}


enum psbt_result
psbt_read(const unsigned char *src, size_t src_size, struct psbt *tx,
	  psbt_record_cb *rec_cb, void* user_data)
{
	struct psbt_record rec;
	enum psbt_result res;
	u8 *end;
	u32 magic;
	u64 size;

	if (tx->state != PSBT_ST_INIT) {
		psbt_errmsg = "psbt_read: psbt not initialized, use psbt_init first";
		return PSBT_INVALID_STATE;
	}

	if (src_size > tx->data_capacity) {
		psbt_errmsg = "psbt_read: read buffer is larger than psbt capacity";
		return PSBT_OUT_OF_BOUNDS_WRITE;
	}

	if (src != tx->data)
		memcpy(tx->data, src, src_size);

	// parsing should try to get this to the finalized state,
	// otherwise it's an invalid psbt
	tx->state = PSBT_ST_INIT;
	tx->write_pos = tx->data + src_size;

	// XXX: needed so that the asserts work properly. this is probably fine...
	tx->data_capacity = src_size;

	end = tx->data + src_size;

	while (tx->state != PSBT_ST_FINALIZED && tx->write_pos <= end) {
		switch(tx->state) {
		case PSBT_ST_INIT:
			res = psbt_read_header(tx);
			if (res != PSBT_OK)
				return res;
			break;

		case PSBT_ST_HEADER:
		case PSBT_ST_GLOBAL:
		case PSBT_ST_INPUTS:
			res = psbt_read_record(tx, src_size, &rec);
			if (res != PSBT_OK)
				return res;
			rec_cb(user_data, &rec);

			if (*tx->write_pos == 0) {
				tx->state = tx->state == PSBT_ST_GLOBAL ?
					PSBT_ST_INPUTS : PSBT_ST_INPUTS_NEW;
			}

			break;

		case PSBT_ST_INPUTS_NEW:
		case PSBT_ST_FINALIZED:
			assert(0); /* missing from original code */
		}
	}

	return PSBT_OK;
}

enum psbt_result
psbt_write_global_record(struct psbt *tx, struct psbt_record *rec) {
	if (tx->state == PSBT_ST_INIT) {
		// write header if we haven't yet
		psbt_write_header(tx);
		tx->state = PSBT_ST_GLOBAL;
	}
	else if (tx->state == PSBT_ST_HEADER) {
		tx->state = PSBT_ST_GLOBAL;
	}
	else if (tx->state != PSBT_ST_GLOBAL) {
		psbt_errmsg = "psbt_write_global_record: you can only write a "
			"global record after psbt_init and before "
			"psbt_write_input_record";
		return PSBT_INVALID_STATE;
	}

	return psbt_write_record(tx, rec);
}

enum psbt_result
psbt_new_input_record_set(struct psbt *tx) {
	enum psbt_result res;
	if (tx->state == PSBT_ST_GLOBAL
	 || tx->state == PSBT_ST_INPUTS_NEW
	 || tx->state == PSBT_ST_INPUTS) {
		res = psbt_close_records(tx);
		if (res != PSBT_OK)
			return res;
		tx->state = PSBT_ST_INPUTS_NEW;
		return PSBT_OK;
	}
	else if (tx->state != PSBT_ST_INPUTS) {
		psbt_errmsg = "psbt_new_input_record_set: "
			"this can only be called after psbt_write_global_record, "
                        "psbt_new_input_record_set, "
			"or psbt_write_input_record";
		return PSBT_INVALID_STATE;
	}

	return psbt_close_records(tx);
}


enum psbt_result
psbt_print(struct psbt *tx, FILE *stream) {
	if (tx->state != PSBT_ST_FINALIZED) {
		psbt_errmsg = "psbt_print: transaction is not finished";
		return PSBT_INVALID_STATE;
	}

	size_t size = psbt_size(tx);
	for (size_t i = 0; i < size; ++i)
		fprintf(stream, "%02x", tx->data[i]);
	printf("\n");

	return PSBT_OK;
}

enum psbt_result
psbt_write_input_record(struct psbt *tx, struct psbt_record *rec) {
	enum psbt_result res;
	if (tx->state == PSBT_ST_GLOBAL) {
		// close global records
		if ((res = psbt_close_records(tx)) != PSBT_OK)
			return res;
		tx->state = PSBT_ST_INPUTS;
	}
	else if (tx->state != PSBT_ST_INPUTS && tx->state != PSBT_ST_INPUTS_NEW) {
		psbt_errmsg = "psbt_write_input_record: attempting to write an "
			"input record before any global records have been written."
			" use psbt_write_global_record first";
		return PSBT_INVALID_STATE;
	}

	return psbt_write_record(tx, rec);
}

