#ifndef PSBT_H
#define PSBT_H

#include <stddef.h>
#include <stdio.h>

enum psbt_result {
	PSBT_OK,
	PSBT_COMPACT_READ_ERROR,
	PSBT_READ_ERROR,
	PSBT_INVALID_STATE,
	PSBT_OOB_WRITE
};

enum psbt_global_type {
	PSBT_GLOBAL_UNSIGNED_TX = 0
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

typedef void (psbt_record_cb)(void *user_data, struct psbt_record *rec);

size_t
psbt_size(struct psbt *tx);

enum psbt_result
psbt_read(const unsigned char *src, size_t src_size, struct psbt *psbt,
	  psbt_record_cb *rec_cb, void* user_data);

enum psbt_result
psbt_decode(const char *src, size_t src_size, unsigned char *dest, size_t dest_size);

const char *
psbt_state_tostr(enum psbt_state state);

const char *
psbt_type_tostr(unsigned char type, enum psbt_scope scope);

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

#define PSBT_MAGIC 0x70736274


extern char *psbt_errmsg;

#endif /* PSBT_H */
