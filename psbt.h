#ifndef PSBT_H
#define PSBT_H

#include <stddef.h>
#include <stdio.h>

enum psbt_result {
	PSBT_OK,
	PSBT_COMPACT_READ_ERROR,
	PSBT_READ_ERROR,
	PSBT_INVALID_STATE,
	PSBT_OUT_OF_BOUNDS_WRITE
};

#define PSBT_GLOBAL_UNSIGNED_TX     0

#define PSBT_IN_NON_WITNESS_UTXO    0
#define PSBT_IN_WITNESS_UTXO        1
#define PSBT_IN_PARTIAL_SIG         2
#define PSBT_IN_SIGHASH_TYPE        3
#define PSBT_IN_REDEEM_SCRIPT       4
#define PSBT_IN_WITNESS_SCRIPT      5
#define PSBT_IN_BIP32_DERIVATION    6
#define PSBT_IN_FINAL_SCRIPTSIG     7
#define PSBT_IN_FINAL_SCRIPTWITNESS 8

#define PSBT_OUT_REDEEM_SCRIPT      0
#define PSBT_OUT_WITNESS_SCRIPT     1
#define PSBT_OUT_BIP32_DERIVATION   2

enum psbt_state {
	PSBT_ST_INIT = 2,
	PSBT_ST_HEADER,
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
	int is_global; // only needed when reading to signify a global record
};

typedef void (psbt_record_cb)(void *user_data, struct psbt_record *rec);

size_t
psbt_size(struct psbt *tx);

enum psbt_result
psbt_read(const unsigned char *src, struct psbt *dest);

enum psbt_result
psbt_write_global_record(struct psbt *tx, struct psbt_record *rec);

enum psbt_result
psbt_write_input_record(struct psbt *tx, struct psbt_record *rec);

enum psbt_result
psbt_new_input_record_set(struct psbt *tx);

enum psbt_result
psbt_init(struct psbt *tx, unsigned char *dest, size_t dest_size);

enum psbt_result
psbt_print(struct psbt *tx, FILE *stream);

enum psbt_result
psbt_finalize(struct psbt *tx);

#define PSBT_MAGIC 0x70736274


extern char *psbt_errmsg;

#endif /* PSBT_H */
