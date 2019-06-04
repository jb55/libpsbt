
#include "psbt.h"
#include "string.h"
#include <inttypes.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

const char *psbt_example = "70736274ff0100a00200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be40000000000feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f877a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740cbe81d988e71de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b9ee0efe9fa039cca459788ac00000000000100df0200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf6000000006a473044022070b2245123e6bf474d60c5b50c043d4c691a5d2435f09a34a7662a9dc251790a022001329ca9dacf280bdf30740ec0390422422c81cb45839457aeb76fc12edd95b3012102657d118d3357b8e0f4c2cd46db7b39f6d9c38d9a70abcb9b2de5dc8dbfe4ce31feffffff02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e13000001012000e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb8230800220202ead596687ca806043edc3de116cdf29d5e9257c196cd055cf698c8d02bf24e9910b4a6ba670000008000000080020000800022020394f62be9df19952c5587768aeb7698061ad2c4a25c894f47d8c162b4d7213d0510b4a6ba6700000080010000800200008000";

void print_json_rec(void *user_data, struct psbt_record *rec) {
	printf("{\n    ");
	printf("\"tx\": \"");

	/* for (int i = 0; i < rec->) */

	printf("\"type\": \"%s\"", psbt_type_tostr(rec->type, rec->scope));
}

static void txid_print(unsigned char *data) {
	for (int i = 31; i >= 0; i--)
		printf("%02x", data[i]);
}

static void hex_print(unsigned char *data, size_t len) {
	for (size_t i = 0; i < len; ++i)
		printf("%02x", data[i]);
}

void print_rec(struct psbt_elem *elem) {
	const char *type_str = NULL;
	struct psbt_record *rec = NULL;
	struct psbt_txelem *txelem = NULL;
	struct psbt_txin *txin = NULL;
	struct psbt_txout *txout = NULL;
	struct psbt_tx *tx = NULL;

	switch (elem->type) {
	case PSBT_ELEM_RECORD:
		rec = elem->elem.rec;
		type_str = psbt_type_tostr(rec->type, rec->scope);
		printf("%s\t%d ", type_str, elem->index);
		if (rec->key_size != 0) {
			hex_print(rec->key, rec->key_size);
			printf(" ");
		}
		hex_print(rec->val, rec->val_size);
		printf("\n");
		break;
	case PSBT_ELEM_TXELEM:
		txelem = elem->elem.txelem;
		type_str = psbt_txelem_type_tostr(txelem->elem_type);
		printf("%s\t", type_str);
		switch (txelem->elem_type) {
		case PSBT_TXELEM_TXIN:
			txin = txelem->elem.txin;
			txid_print(txin->txid);
			printf(" ind:%d", txin->index);
			printf(" seq:%u", txin->sequence_number);
			if (txin->script_len) {
				printf(" ");
				hex_print(txin->script, txin->script_len);
			}
			printf("\n");
			break;
		case PSBT_TXELEM_TXOUT:
			txout = txelem->elem.txout;
			if (txout->script_len) {
				hex_print(txout->script, txout->script_len);
				printf(" ");
			}
			printf("amount:%"PRIu64"\n", txout->amount);
			break;
		case PSBT_TXELEM_TX:
			tx = txelem->elem.tx;
			printf("ver:%u locktime:%u\n", tx->version, tx->lock_time);
			break;
		default:
			break;
		}
	}
}

int usage() {
	printf ("usage: psbt <psbt>\n");
	return 1;
}

#define CHECK(res) \
	if ((res) != PSBT_OK) {					\
		printf("error (%d): %s. last_state = %s\n", res, psbt_errmsg, \
		       psbt_state_tostr(psbt.state));			\
		return 1;					\
	}


int main(int argc, char *argv[])
{
	struct psbt psbt;
	static unsigned char buffer[4096];
	enum psbt_result res;
	size_t psbt_len;
	size_t out_len;

	if (argc < 2)
		return usage();

	size_t psbt_hex_len = strlen(argv[1]);

	psbt_init(&psbt, buffer, 4096);

	res = psbt_decode(argv[1], psbt_hex_len, buffer, 4096, &psbt_len);
	CHECK(res);

	res = psbt_read(buffer, psbt_len, &psbt, print_rec, NULL);
	CHECK(res);

	res = psbt_encode(&psbt, PSBT_ENCODING_BASE62, buffer, 4096, &out_len);
	CHECK(res);

	/* printf("%.*s", (int)out_len, buffer); */

	/* printf("%.*s\n", (int)out_len, (char*)buffer); */

	return 0;
}
