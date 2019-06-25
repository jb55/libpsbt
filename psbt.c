
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <endian.h>
#include <assert.h>
#include <inttypes.h>
#include "compactsize.h"
#include "tx.h"
#include "base64.h"

#ifdef DEBUG
  #define debug(...) fprintf(stderr, __VA_ARGS__)
#else
  #define debug(...)
#endif

char *psbt_errmsg = NULL;

const unsigned char PSBT_MAGIC[4] = {0x70, 0x73, 0x62, 0x74};

#define ASSERT_SPACE(s) \
	if (tx->write_pos+(s) > tx->data + tx->data_capacity) { \
		psbt_errmsg = "write out of bounds " __FILE__ ":" STRINGIZE(__LINE__); \
		return PSBT_OOB_WRITE; \
	}

size_t psbt_size(struct psbt *tx) {
	return tx->write_pos - tx->data;
}

static void hex_print(unsigned char *data, size_t len) {
	for (size_t i = 0; i < len; ++i)
		printf("%02x", data[i]);
}



static enum psbt_result
psbt_write_header(struct psbt *tx) {
	ASSERT_SPACE(sizeof(PSBT_MAGIC));
	memcpy(tx->write_pos, PSBT_MAGIC, sizeof(PSBT_MAGIC));
	tx->write_pos += sizeof(PSBT_MAGIC);

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

	if (tx->state != PSBT_ST_OUTPUTS_NEW && tx->state != PSBT_ST_OUTPUTS) {
		psbt_errmsg = "psbt_finalize: no output records found";
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

	debug("magic %02X %02X %02X %02X\n",
		*tx->write_pos,
		*(tx->write_pos+1),
		*(tx->write_pos+2),
		*(tx->write_pos+3));

	if (memcmp(tx->write_pos, PSBT_MAGIC, sizeof(PSBT_MAGIC)) != 0) {
		psbt_errmsg = "psbt_read: invalid magic header";
		return PSBT_READ_ERROR;
	}

	tx->write_pos += 4;

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
psbt_txelem_type_tostr(enum psbt_txelem_type txelem_type) {
	switch (txelem_type) {
	case PSBT_TXELEM_TX:
		return "TX";
	case PSBT_TXELEM_TXIN:
		return "TXIN";
	case PSBT_TXELEM_TXOUT:
		return "TXOUT";
	case PSBT_TXELEM_WITNESS_ITEM:
		return "WITNESS_ITEM";
	}

	return "UNKNOWN_TXELEM";
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
	assert(size > 0);

	tx->write_pos += size_len;

	debug("record pos %zu size_len %d size %"PRIu64"\n",
	      tx->write_pos - tx->data, size_len, size);

	if (res != PSBT_OK)
		return res;

	if (tx->write_pos + size > tx->data + src_size) {
		psbt_errmsg = "psbt_read: record key size too large";
		return PSBT_READ_ERROR;
	}

	ASSERT_SPACE(size);

	rec->key_size = size - 1; // don't include type in key size
	rec->type = *tx->write_pos;
	rec->key = tx->write_pos + 1;

	tx->write_pos += size;
	debug("%02x %02x %02x %02x %02x\n", *(tx->write_pos-2), *(tx->write_pos-1),
	      *tx->write_pos, *(tx->write_pos+1), *(tx->write_pos+2));

	debug("record pos %zu size_len %d size %"PRIu64"\n",
	      tx->write_pos - tx->data, size_len, size);

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

struct psbt_tx_counter {
	int inputs;
	int outputs;
	void *user_data;
	psbt_elem_handler *handler;
} counter;

static void tx_counter(struct psbt_txelem *elem) {
	struct psbt_elem psbt_elem;
	struct psbt_tx_counter *counter =
		(struct psbt_tx_counter *)elem->user_data;

	psbt_elem.user_data = counter->user_data;
	psbt_elem.type = PSBT_ELEM_TXELEM;
	psbt_elem.elem.txelem = elem;

	// forward txelem events to user
	if (counter->handler)
		counter->handler(&psbt_elem);

	switch (elem->elem_type) {
	case PSBT_TXELEM_TXIN:
		counter->inputs++;
		return;
	case PSBT_TXELEM_TXOUT:
		counter->outputs++;
		return;
	default:
		return;
	}
}

enum psbt_result
psbt_read(const unsigned char *src, size_t src_size, struct psbt *tx,
	  psbt_elem_handler *elem_handler, void* user_data)
{
	struct psbt_record rec;
	enum psbt_result res;

	struct psbt_elem elem;
	elem.type = PSBT_ELEM_RECORD;
	elem.user_data = user_data;

	int kvs = 0;
	u8 *end;

	struct psbt_tx_counter counter = {
		.inputs = 0,
		.outputs = 0,
		.user_data = user_data,
		.handler = elem_handler,
	};

	if (tx->state != PSBT_ST_INIT) {
		psbt_errmsg = "psbt_read: psbt not initialized, use psbt_init first";
		return PSBT_INVALID_STATE;
	}

	if (src_size > tx->data_capacity) {
		psbt_errmsg = "psbt_read: read buffer is larger than psbt capacity";
		return PSBT_OOB_WRITE;
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
			debug("reading header at %zu\n", tx->write_pos - tx->data);
			res = psbt_read_header(tx);
			if (res != PSBT_OK)
				return res;
			break;

		case PSBT_ST_GLOBAL:
		case PSBT_ST_INPUTS:
		case PSBT_ST_OUTPUTS:

			if (*tx->write_pos == 0) {
				switch (tx->state) {
				case PSBT_ST_GLOBAL:
					tx->state = PSBT_ST_INPUTS_NEW;
					break;

				case PSBT_ST_INPUTS:
					if (++kvs >= counter.inputs) {
						tx->state = PSBT_ST_OUTPUTS_NEW;
						kvs = 0;
					} else
						tx->state = PSBT_ST_INPUTS_NEW;
					break;

				case PSBT_ST_OUTPUTS:
					if (++kvs >= counter.outputs)
						tx->state = PSBT_ST_FINALIZED;
					else
						tx->state = PSBT_ST_OUTPUTS_NEW;
					break;

				default:
					assert(!"psbt_read: invalid state at null byte");
				}
			}
			else {
				debug("reading record @ %zu\n", tx->write_pos - tx->data);
				res = psbt_read_record(tx, src_size, &rec);

				if (res != PSBT_OK)
					return res;

				if (tx->state == PSBT_ST_GLOBAL &&
				    rec.type == PSBT_GLOBAL_UNSIGNED_TX) {
					// parse transaction for number of inputs/outputs
					res = psbt_btc_tx_parse(rec.val,
								rec.val_size,
								(void*)&counter,
								tx_counter);

					if (res != PSBT_OK)
						return res;
				}

				// record callback
				if (elem_handler) {
					elem.type = PSBT_ELEM_RECORD;
					elem.index = kvs;
					elem.elem.rec = &rec;
					elem_handler(&elem);
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
			assert(!"impossible");
			break;
		}
	}

	if (tx->state != PSBT_ST_FINALIZED) {
		psbt_errmsg = "psbt_read: invalid psbt";
		return PSBT_INVALID_STATE;
	}
	else if (tx->state == PSBT_ST_FINALIZED && *tx->write_pos != 0) {
		psbt_errmsg = "psbt_read: expected null byte at end of psbt";
		return PSBT_READ_ERROR;
	}

	tx->write_pos++;

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


enum psbt_result
psbt_write_output_record(struct psbt *tx, struct psbt_record *rec) {
	enum psbt_result res;
	if (tx->state == PSBT_ST_INPUTS) {
		// close global records
		if ((res = psbt_close_records(tx)) != PSBT_OK)
			return res;
		tx->state = PSBT_ST_OUTPUTS;
	}
	else if (tx->state != PSBT_ST_OUTPUTS && tx->state != PSBT_ST_OUTPUTS_NEW) {
		psbt_errmsg = "psbt_write_input_record: attempting to write an "
			"input record before any global records have been written."
			" use psbt_write_global_record first";
		return PSBT_INVALID_STATE;
	}

	return psbt_write_record(tx, rec);
}


static inline u8 hexdigit( char hex ) {
	return (hex <= '9') ? hex - '0' : toupper(hex) - 'A' + 10 ;
}

enum psbt_result
psbt_hex_decode(const char *src, size_t src_size, unsigned char *dest,
	        size_t dest_size) {

	enum psbt_result res = PSBT_OK;

	if (src_size % 2 != 0) {
		psbt_errmsg = "psbt_decode: invalid hex string";
		return PSBT_READ_ERROR;
	}

	if (dest_size < src_size / 2) {
		psbt_errmsg = "psbt_decode: dest_size must be at least half the"
			" size of src_size";
		return PSBT_READ_ERROR;
	}

	if (res != PSBT_OK)
		return res;

	for (size_t i = 0; i < src_size; i += 2) {
		u8 c1 = src[i];
		u8 c2 = src[i+1];
		if (!isxdigit(c1) || !isxdigit(c2)) {
			psbt_errmsg = "psbt_decode: invalid hex string";
			return PSBT_READ_ERROR;
		}

		*dest++ = hexdigit(c1) << 4 | hexdigit(c2);
	}

	return PSBT_OK;
}

enum psbt_result
psbt_decode(const char *src, size_t src_size, unsigned char *dest,
	    size_t dest_size, size_t *psbt_size) {
	enum psbt_result res;
	// TODO: detect base64 or hex encoding
	// default is hex encoding for now

	size_t b64_magic_size = sizeof("cHNid") - 1;

	// simple sanity check before we look for base64 encoding
	if (src_size < b64_magic_size) {
		psbt_errmsg = "psbt_decode: psbt too small";
		return PSBT_READ_ERROR;
	}

	// base64 detection
	if (memcmp(src, "cHNid", b64_magic_size) == 0) {
		u8 *c = base64_decode((unsigned char*)src, src_size, dest,
					dest_size, psbt_size);
		return c == NULL ? PSBT_READ_ERROR : PSBT_OK;
	}

	*psbt_size = src_size / 2;
	return psbt_hex_decode(src, src_size, dest, dest_size);
}

static char hexchar(unsigned int val)
{
	if (val < 10)
		return '0' + val;
	if (val < 16)
		return 'a' + val - 10;
	assert(!"hexchar invalid val");
}

static enum psbt_result
hex_encode(u8 *buf, size_t bufsize, u8 *dest, size_t dest_size) {
	size_t i;

	if (dest_size < bufsize * 2 + 1)
		return PSBT_OOB_WRITE;

	for (i = 0; i < bufsize; i++) {
		unsigned int c = buf[i];
		*(dest++) = hexchar(c >> 4);
		*(dest++) = hexchar(c & 0xF);
	}
	*dest = '\0';

	return PSBT_OK;
}

/* static enum psbt_result */
/* base64_encode(u8 *psbt, size_t psbt_size, u8 *dest, size_t dest_size) { */
/* 	return PSBT_NOT_IMPLEMENTED; */
/* } */

static enum psbt_result
protobuf_encode(u8 *psbt, size_t psbt_size, u8 *dest, size_t dest_size) {
	return PSBT_NOT_IMPLEMENTED;
}

enum psbt_result
psbt_encode_raw(unsigned char *psbt_data, size_t psbt_len,
		enum psbt_encoding encoding, unsigned char *dest,
		size_t dest_size, size_t* out_len)
{
	u8 *c;
	enum psbt_result res;

	switch (encoding) {
	case PSBT_ENCODING_HEX:
		res = hex_encode(psbt_data, psbt_len, dest, dest_size);
		*out_len = psbt_len * 2 + 1;
		return res;
	case PSBT_ENCODING_BASE64:
		c = base64_encode(psbt_data, psbt_len, dest, dest_size, out_len);
		if (c == NULL) {
			psbt_errmsg = "psbt_encode: base64 encode failure";
			return PSBT_WRITE_ERROR;
		}
		return PSBT_OK;
	case PSBT_ENCODING_BASE62:
		c = base62_encode(psbt_data, psbt_len, dest, dest_size, out_len);
		if (c == NULL) {
			psbt_errmsg = "psbt_encode: base62 encode failure";
			return PSBT_WRITE_ERROR;
		}
		return PSBT_OK;
	case PSBT_ENCODING_PROTOBUF:
		return protobuf_encode(psbt_data, psbt_len, dest, dest_size);
	}

	psbt_errmsg = "psbt_encode: invalid psbt_encoding enum value";
	return PSBT_NOT_IMPLEMENTED;
}

enum psbt_result
psbt_encode(struct psbt *psbt, enum psbt_encoding encoding, unsigned char *dest,
	    size_t dest_size, size_t *out_len)
{
	if (psbt->state != PSBT_ST_FINALIZED) {
		psbt_errmsg = "psbt_encode: psbt not in finalized state. "
			"use psbt_read to parse an existing psbt, or the "
			"psbt_write functions to create one.";

		return PSBT_WRITE_ERROR;
	}

	return psbt_encode_raw(psbt->data, psbt_size(psbt), encoding, dest,
			       dest_size, out_len);
}


const char *
psbt_geterr() {
	return psbt_errmsg;
}
