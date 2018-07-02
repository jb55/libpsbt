
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <assert.h>
#include "compactsize.h"

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

	tx->state = PSBT_ST_GLOBAL;

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

	if (tx->state != PSBT_ST_INPUTS_NEW && tx->state != PSBT_ST_INPUTS) {
		psbt_errmsg = "psbt_finalize: no input records found";
		return PSBT_INVALID_STATE;
	}

	res = psbt_close_records(tx);
	if (res != PSBT_OK)
		return res;

	tx->state = PSBT_ST_FINALIZED;

	return PSBT_OK;
}

static enum psbt_result
psbt_write_record(struct psbt *tx, struct psbt_record *rec) {
	u32 size;
	u32 key_size_with_type = rec->key_size + 1;

	// write key length
	size = compactsize_length(key_size_with_type);
	ASSERT_SPACE(size);
	compactsize_write((u8*)tx->write_pos, key_size_with_type);
	tx->write_pos += size;

	// write type
	ASSERT_SPACE(1);
	*tx->write_pos++ = rec->type;

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

const char *
psbt_input_type_tostr(enum psbt_input_type type) {
	switch (type) {
	case PSBT_IN_NON_WITNESS_UTXO:
		return "IN_NON_WITNESS_UTXO";
	case PSBT_IN_WITNESS_UTXO:
		return "IN_WITNESS_UTXO";
	case PSBT_IN_PARTIAL_SIG:
		return "IN_PARTIAL_SIG";
	case PSBT_IN_SIGHASH_TYPE:
		return "IN_SIGHASH_TYPE";
	case PSBT_IN_REDEEM_SCRIPT:
		return "IN_REDEEM_SCRIPT";
	case PSBT_IN_WITNESS_SCRIPT:
		return "IN_WITNESS_SCRIPT";
	case PSBT_IN_BIP32_DERIVATION:
		return "IN_BIP32_DERIVATION";
	case PSBT_IN_FINAL_SCRIPTSIG:
		return "IN_FINAL_SCRIPTSIG";
	case PSBT_IN_FINAL_SCRIPTWITNESS:
		return "IN_FINAL_SCRIPTWITNESS";
	}

	return "UNKNOWN_INPUT_TYPE";
}

const char *
psbt_state_tostr(enum psbt_state state) {
	switch (state) {
	case PSBT_ST_INIT:
		return "INIT";
	case PSBT_ST_GLOBAL:
		return "GLOBAL";
	case PSBT_ST_INPUTS:
		return "INPUTS";
	case PSBT_ST_INPUTS_NEW:
		return "INPUTS_NEW";
	case PSBT_ST_OUTPUTS:
		return "OUTPUTS";
	case PSBT_ST_OUTPUTS_NEW:
		return "OUTPUTS_NEW";
	case PSBT_ST_FINALIZED:
		return "FINALIZED";
	}

	return "UNKNOWN_STATE";
}

const char *
psbt_output_type_tostr(enum psbt_output_type type) {
	switch (type) {
	case PSBT_OUT_REDEEM_SCRIPT:
		return "OUT_REDEEM_SCRIPT";
	case PSBT_OUT_WITNESS_SCRIPT:
		return "OUT_WITNESS_SCRIPT";
	case PSBT_OUT_BIP32_DERIVATION:
		return "OUT_BIP32_DERIVATION";
	}

	return "UNKNOWN_OUTPUT_TYPE";
}

const char *
psbt_global_type_tostr(enum psbt_global_type type) {
	switch (type) {
	case PSBT_GLOBAL_UNSIGNED_TX: return "GLOBAL_UNSIGNED_TX";
	}

	return "UNKNOWN_GLOBAL_TYPE";
}

const char *
psbt_type_tostr(unsigned char type, enum psbt_scope scope) {
	switch (scope) {
	case PSBT_SCOPE_GLOBAL:
		return psbt_global_type_tostr((enum psbt_global_type)type);
	case PSBT_SCOPE_INPUTS:
		return psbt_input_type_tostr((enum psbt_input_type)type);
	case PSBT_SCOPE_OUTPUTS:
		return psbt_output_type_tostr((enum psbt_output_type)type);
	}

	return "UNKNOWN_SCOPE";
}


static enum psbt_result
psbt_read_record(struct psbt *tx, size_t src_size, struct psbt_record *rec)
{
	enum psbt_result res = PSBT_OK;
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
	switch (tx->state) {
		case PSBT_ST_GLOBAL:
			rec->scope = PSBT_SCOPE_GLOBAL;
			break;

		case PSBT_ST_INPUTS:
			rec->scope = PSBT_SCOPE_INPUTS;
			break;

		case PSBT_ST_OUTPUTS:
			rec->scope = PSBT_SCOPE_OUTPUTS;
			break;

		default:
			psbt_errmsg = "psbt_read_record: invalid record state";
			return PSBT_INVALID_STATE;
	}

	rec->type = *tx->write_pos;

	tx->write_pos += size;

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
	tx->write_pos = tx->data;

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

		case PSBT_ST_GLOBAL:
		case PSBT_ST_INPUTS:
		case PSBT_ST_OUTPUTS:
			res = psbt_read_record(tx, src_size, &rec);

			if (res != PSBT_OK)
				return res;

			rec_cb(user_data, &rec);

			if (*tx->write_pos == 0) {
				switch (tx->state) {
				case PSBT_ST_GLOBAL:
					tx->state = PSBT_ST_INPUTS_NEW;
					break;

				case PSBT_ST_INPUTS:
					tx->state = PSBT_ST_OUTPUTS_NEW;
					break;

				case PSBT_ST_OUTPUTS:
					tx->state = PSBT_ST_FINALIZED;
					break;

				default:
					assert(!"psbt_read: invalid state at null byte");
				}
			}
			break;

		case PSBT_ST_OUTPUTS_NEW:
			assert(*tx->write_pos == 0);
			tx->write_pos++;
			tx->state = PSBT_ST_OUTPUTS;
			break;

		case PSBT_ST_INPUTS_NEW:
			assert(*tx->write_pos == 0);
			tx->write_pos++;
			tx->state = PSBT_ST_INPUTS;
			break;

		case PSBT_ST_FINALIZED:
			break;
		}
	}

	if (tx->state != PSBT_ST_FINALIZED) {
		psbt_errmsg = "psbt_read: invalid psbt";
		return PSBT_READ_ERROR;
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
psbt_new_output_record_set(struct psbt *tx) {
	enum psbt_result res;
	if (tx->state == PSBT_ST_INPUTS
	    || tx->state == PSBT_ST_INPUTS_NEW
	    || tx->state == PSBT_ST_OUTPUTS_NEW
	    || tx->state == PSBT_ST_OUTPUTS)
	{
		res = psbt_close_records(tx);
		if (res != PSBT_OK)
			return res;
		tx->state = PSBT_ST_OUTPUTS_NEW;
		return PSBT_OK;
	}
	else if (tx->state != PSBT_ST_OUTPUTS) {
		psbt_errmsg = "psbt_new_output_record_set: "
			"this can only be called after writing input records";
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

