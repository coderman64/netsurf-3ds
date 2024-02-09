#include <stdio.h>

#include "rufl.h"

#include "harness.h"
#include "testutils.h"

int main(int argc, const char **argv)
{
	UNUSED(argc);
	UNUSED(argv);

	rufl_test_harness_init(380, true, false);

	assert(rufl_FONT_MANAGER_ERROR == rufl_init());

	printf("PASS\n");

	return 0;
}
