
#ifndef PSBT_TX_H
#define PSBT_TX_H



#include <stdint.h>
#include "result.h"

struct psbt_txin {
	unsigned char *txid;
	unsigned int index; /* output number referred to by above */
	unsigned char *script;
	unsigned int script_len;
	unsigned int sequence_number;
};


struct psbt_txout {
	uint64_t amount;
	unsigned char *script;
	unsigned int script_len;
};

struct psbt_witness_item {
	int input_index;
	int item_index;
	unsigned char *item;
	unsigned int item_len;
};

struct psbt_tx {
	unsigned int version;
	unsigned int lock_time;
};

enum psbt_txelem_type {
	PSBT_TXELEM_TXIN,
	PSBT_TXELEM_TXOUT,
	PSBT_TXELEM_TX,
	PSBT_TXELEM_WITNESS_ITEM,
};

struct psbt_txelem {
	enum psbt_txelem_type elem_type;
	void *user_data;
	union {
		struct psbt_txin *txin;
		struct psbt_txout *txout;
		struct psbt_tx *tx;
		struct psbt_witness_item *witness_item;
	} elem;
};

typedef void (psbt_txelem_handler)(struct psbt_txelem *handler);


enum psbt_result
psbt_btc_tx_parse(unsigned char *tx, unsigned int tx_size, void *user_data,
		  psbt_txelem_handler *handler);


#endif /* PSBT_TX_H */
