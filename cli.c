
#include "psbt.h"

int main(int argc, char *argv[])
{
	struct psbt tx;
	static unsigned char buffer[2048];

	psbt_init(&tx, buffer, 2048);

	psbt_read(psbt_example, &tx);


	return 0;
}
