#ifndef PSBT_H
#define PSBT_H

#include <stddef.h>
#include <stdio.h>
#include "result.h"
#include "tx.h"

enum psbt_global_type {
	PSBT_GLOBAL_UNSIGNED_TX = 0
};

enum psbt_encoding {
	PSBT_ENCODING_HEX,
	PSBT_ENCODING_BASE64,
	PSBT_ENCODING_BASE62,
	PSBT_ENCODING_PROTOBUF,
};

enum psbt_input_type {
	PSBT_IN_NON_WITNESS_UTXO    = 0,
	PSBT_IN_WITNESS_UTXO        = 1,
	PSBT_IN_PARTIAL_SIG         = 2,
	PSBT_IN_SIGHASH_TYPE        = 3,
	PSBT_IN_REDEEM_SCRIPT       = 4,
	PSBT_IN_WITNESS_SCRIPT      = 5,
	PSBT_IN_BIP32_DERIVATION    = 6,
	PSBT_IN_FINAL_SCRIPTSIG     = 7,
	PSBT_IN_FINAL_SCRIPTWITNESS = 8,
};


enum psbt_output_type {
	PSBT_OUT_REDEEM_SCRIPT      = 0,
	PSBT_OUT_WITNESS_SCRIPT     = 1,
	PSBT_OUT_BIP32_DERIVATION   = 2,
};

enum psbt_scope {
	PSBT_SCOPE_GLOBAL,
	PSBT_SCOPE_INPUTS,
	PSBT_SCOPE_OUTPUTS,
};

enum psbt_state {
	PSBT_ST_INIT = 2,
	PSBT_ST_GLOBAL,
	PSBT_ST_INPUTS,
	PSBT_ST_INPUTS_NEW,
	PSBT_ST_OUTPUTS,
	PSBT_ST_OUTPUTS_NEW,
	PSBT_ST_FINALIZED,
};

struct psbt {
	enum psbt_state state;
	unsigned char *data;
	unsigned char *write_pos;
	size_t data_capacity;
};

struct psbt_record {
	unsigned char type;
	unsigned char *key;
	unsigned int key_size;
	unsigned char *val;
	unsigned int val_size;
	enum psbt_scope scope;
};

enum psbt_elem_type {
	PSBT_ELEM_RECORD,
	PSBT_ELEM_TXELEM,
};

struct psbt_elem {
	enum psbt_elem_type type;
	void *user_data;
	int index;
	union {
		struct psbt_txelem *txelem;
		struct psbt_record *rec;
	} elem;
};

typedef void (psbt_elem_handler)(struct psbt_elem *rec);

size_t
psbt_size(struct psbt *tx);

enum psbt_result
psbt_read(const unsigned char *src, size_t src_size, struct psbt *psbt,
	  psbt_elem_handler *elem_handler, void* user_data);

enum psbt_result
psbt_decode(const char *src, size_t src_size, unsigned char *dest,
	    size_t dest_size, size_t *psbt_len);

enum psbt_result
psbt_encode(struct psbt *psbt, enum psbt_encoding encoding, unsigned char *dest,
	    size_t dest_size, size_t *out_len);

enum psbt_result
psbt_encode_raw(unsigned char *psbt_data, size_t psbt_len,
		enum psbt_encoding encoding, unsigned char *dest,
		size_t dest_size, size_t* out_len);

const char *
psbt_geterr();

const char *
psbt_state_tostr(enum psbt_state state);

const char *
psbt_type_tostr(unsigned char type, enum psbt_scope scope);

const char *
psbt_txelem_type_tostr(enum psbt_txelem_type txelem_type);

const char *
psbt_global_type_tostr(enum psbt_global_type type);

const char *
psbt_output_type_tostr(enum psbt_output_type type);

const char *
psbt_input_type_tostr(enum psbt_input_type type);

enum psbt_result
psbt_write_global_record(struct psbt *tx, struct psbt_record *rec);

enum psbt_result
psbt_write_input_record(struct psbt *tx, struct psbt_record *rec);

enum psbt_result
psbt_write_output_record(struct psbt *tx, struct psbt_record *rec);

enum psbt_result
psbt_new_input_record_set(struct psbt *tx);

enum psbt_result
psbt_new_output_record_set(struct psbt *tx);

enum psbt_result
psbt_init(struct psbt *tx, unsigned char *dest, size_t dest_size);

enum psbt_result
psbt_print(struct psbt *tx, FILE *stream);

enum psbt_result
psbt_finalize(struct psbt *tx);

extern const unsigned char PSBT_MAGIC[4];

extern char *psbt_errmsg;

#endif /* PSBT_H */
