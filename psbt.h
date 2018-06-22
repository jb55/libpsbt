
#ifndef PSBT_H
#define PSBT_H

#include <stddef.h>

enum psbt_result {
	PSBT_OK,
	PSBT_COMPACT_READ_ERROR,
	PSBT_INVALID_STATE,
	PSBT_OUT_OF_BOUNDS_WRITE
};

enum psbt_state {
	PSBT_ST_INIT = 2,
	PSBT_ST_GLOBAL,
	PSBT_ST_INPUTS,
	PSBT_ST_INPUTS_NEW,
	PSBT_ST_FINISHED,
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
};

size_t
psbt_size(struct psbt *tx);

enum psbt_result
psbt_write_global_record(struct psbt *tx, struct psbt_record *rec);

enum psbt_result
psbt_write_input_record(struct psbt *tx, struct psbt_record *rec);

enum psbt_result
psbt_init(struct psbt *tx, unsigned char *dest, size_t dest_size);

enum psbt_result
psbt_finalize(struct psbt *tx);

#define PSBT_MAGIC 0x70736274

#define PSBT_TYPE_TRANSACTION   0x00
#define PSBT_TYPE_REDEEM_SCRIPT 0x01
#define PSBT_TYPE_PUBLIC_KEY    0x02
#define PSBT_TYPE_NUM_INPUTS    0x03

#define PSBT_INPUT_NON_WITNESS_UTXO 0x00
#define PSBT_INPUT_WITNESS_UTXO     0x01
#define PSBT_INPUT_PARTIAL_SIG      0x02
#define PSBT_INPUT_SIGHASH_TYPE     0x03
#define PSBT_INPUT_INDEX            0x04


extern char *psbt_errmsg;

#endif /* PSBT_H */

