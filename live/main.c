// Note: Only use 7-bit control channnels 64-119 for easy FL Studio compatibility

#include <stdio.h>
#include <stdlib.h>

#include "vendor.h"

#define W4ON_INST_COUNT 1
#include "../w4on.h"

w4on_seq_t seq;

int main(int argc, char **argv)
{
	printf("%lld\n", sizeof(seq));
	// (   = ´  _ `=)

	return 0;
}
