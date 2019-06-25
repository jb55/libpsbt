
#define _DEFAULT_SOURCE 1

#include "tx.h"
#include "result.h"
#include "compactsize.h"
#include "common.h"
#include <endian.h>
#include <assert.h>
#include <string.h>


#ifdef DEBUG
#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug(...)
#endif


#define SEGREGATED_WITNESS_FLAG 0x1

#define ASSERT_SPACE(s)							\
	if (p+(s) > data + data_size) {		\
		psbt_errmsg = "out of bounds " __FILE__ ":" STRINGIZE(__LINE__); \
		return PSBT_READ_ERROR; \
	}

static u32
parse_le32(const u8 *cursor) {
	return le32toh(*(u32*)cursor);
}

static u64
parse_le64(const u8 *cursor) {
	return le64toh(*(u64*)cursor);
}

static enum psbt_result
parse_txin(u8 **cursor, u8 *data, u32 data_size, struct psbt_txin *txin) {
	u64 script_len;
	int size_len;
	enum psbt_result res = PSBT_OK;

	u8 *p = *cursor;

	ASSERT_SPACE(32);
	txin->txid = p;
	p += 32;

	ASSERT_SPACE(4);
	txin->index = parse_le32(p);
	p += 4;

	ASSERT_SPACE(1);
	size_len = compactsize_peek_length(*p);

	ASSERT_SPACE(size_len);
	script_len = compactsize_read(p, &res);
	if (res != PSBT_OK)
		return res;

	p += size_len;

	txin->script_len = script_len;
	txin->script = script_len ? p : NULL;

	p += script_len;

	ASSERT_SPACE(4);
	txin->sequence_number = parse_le32(p);
	p += 4;

	*cursor = p;

	return PSBT_OK;
}


static enum psbt_result
parse_txout(u8 **cursor, u8 *data, u32 data_size, struct psbt_txout *txout) {
	size_t size_len;
	u64 script_len;
	enum psbt_result res = PSBT_OK;

	u8 *p = *cursor;

	ASSERT_SPACE(8);
	txout->amount = parse_le64(p);
	p += 8;

	ASSERT_SPACE(1);
	size_len = compactsize_peek_length(*p);

	ASSERT_SPACE(size_len);
	script_len = compactsize_read(p, &res);
	if (res != PSBT_OK)
		return res;

	p += size_len;

	txout->script = p;
	txout->script_len = script_len;

	ASSERT_SPACE(script_len);
	p += script_len;

	*cursor = p;

	return PSBT_OK;
}

static enum psbt_result
parse_witness_item(u8 **cursor, u8 *data, u32 data_size,
		   struct psbt_witness_item *witness_item) {
	u32 size_len, item_len;
	enum psbt_result res = PSBT_OK;

	u8 *p = *cursor;

	ASSERT_SPACE(1);
	size_len = compactsize_peek_length(*p);

	ASSERT_SPACE(size_len);
	item_len = compactsize_read(p, &res);
	if (res != PSBT_OK)
		return res;

	p += size_len;

	witness_item->item = p;
	witness_item->item_len = item_len;

	p += item_len;

	*cursor = p;

	return PSBT_OK;
}


enum psbt_result
psbt_btc_tx_parse(u8 *data, u32 data_size, void *user_data,
		  psbt_txelem_handler *handler) {
	struct psbt_tx tx;
	enum psbt_result res = PSBT_OK;
	struct psbt_txin txin;
	struct psbt_txout txout;
	struct psbt_txelem txelem;
	struct psbt_witness_item wi;

	u64 count = 0;
	u8 flag = 0;
	u32 size_len = 0;
	size_t inputs = 0;
	size_t i = 0, j = 0;

	u8 *p = data;
	txelem.user_data = user_data;

	ASSERT_SPACE(4);
	tx.version = parse_le32(p);
	p += 4;

	ASSERT_SPACE(1);
	size_len = compactsize_peek_length(*p);

	ASSERT_SPACE(size_len);
	count = compactsize_read(p, &res);
	if (res != PSBT_OK)
		return res;

	p += size_len;

	inputs = count;
	debug("parsing %zu inputs\n", count);

	// parse inputs
	for (i = 0; i < count; i++) {
		res = parse_txin(&p, data, data_size, &txin);
		if (res != PSBT_OK)
			return res;
		txelem.elem_type = PSBT_TXELEM_TXIN;
		txelem.elem.txin = &txin;
		handler(&txelem);
	}

	ASSERT_SPACE(1);
	size_len = compactsize_peek_length(*p);

	ASSERT_SPACE(size_len);
	count = compactsize_read(p, &res);
	if (res != PSBT_OK)
		return res;

	p += size_len;

	// parse outputs
	for (i = 0; i < count; i++) {
		res = parse_txout(&p, data, data_size, &txout);
		if (res != PSBT_OK)
			return res;
		txelem.elem_type = PSBT_TXELEM_TXOUT;
		txelem.elem.txout = &txout;
		handler(&txelem);
	}

	if (flag & SEGREGATED_WITNESS_FLAG) {
		for (i = 0; i < inputs; i++) {
			ASSERT_SPACE(1);
			size_len = compactsize_peek_length(*p);

			ASSERT_SPACE(size_len);
			count = compactsize_read(p, &res);
			if (res != PSBT_OK)
				return res;

			p += size_len;

			for (j = 0; j < count; j++) {
				res = parse_witness_item(&p, data, data_size, &wi);
				if (res != PSBT_OK)
					return res;
				wi.input_index = i;
				wi.item_index = j;
				txelem.elem_type = PSBT_TXELEM_WITNESS_ITEM;
				txelem.elem.witness_item = &wi;
				handler(&txelem);
			}

		}
	}

	ASSERT_SPACE(4);
	tx.lock_time = parse_le32(p);
	p += 4;

	if (p != data + data_size) {
		psbt_errmsg = "psbt_btc_tx_parse: parsing fell short";
		return PSBT_READ_ERROR;
	}

	txelem.elem_type = PSBT_TXELEM_TX;
	txelem.elem.tx = &tx;
	handler(&txelem);

	return PSBT_OK;
}
