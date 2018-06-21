
#ifndef PSBT_H
#define PSBT_H

enum psbt_result {
	PSBT_OK,
	PSBT_COMPACT_READ_ERROR,
	PSBT_OUT_OF_BOUNDS_WRITE
};

struct psbt_transaction {
	char *data;
	char *write_pos;
	size_t data_size;
};

#define PSBT_MAGIC 0x70736274

#define PSBT_GLOBAL_TRANSACTION   0x00
#define PSBT_GLOBAL_REDEEM_SCRIPT 0x01
#define PSBT_GLOBAL_PUBLIC_KEY    0x02
#define PSBT_GLOBAL_NUM_INPUTS    0x03

#define PSBT_INPUT_NON_WITNESS_UTXO 0x00
#define PSBT_INPUT_WITNESS_UTXO     0x01
#define PSBT_INPUT_PARTIAL_SIG      0x02
#define PSBT_INPUT_SIGHASH_TYPE     0x03
#define PSBT_INPUT_INDEX            0x04


extern char *psbt_errmsg;

#endif /* PSBT_H */

