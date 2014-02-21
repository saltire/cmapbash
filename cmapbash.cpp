
#include "nbt.h"

#include <errno.h>
#include <iostream>
#include <stdio.h>

int main(int argc, char **argv) {
	if (argc > 1)
	{
	    FILE* f = fopen(argv[1], "rb");
		nbt_node* root = nbt_parse_file(f);
		fclose(f);
	}

    if (errno != NBT_OK)
    {
        fprintf(stderr, "Parsing error!\n");
        return 0;
    }

	return 0;
}
