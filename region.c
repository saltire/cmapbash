#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "nbt.h"

#include "render.h"


#define REGIONWIDTH 32
#define REGIONAREA REGIONWIDTH * REGIONWIDTH
#define SECTORSIZE 4096


int render_region(char* filename)
{
	FILE* region = fopen(filename, "r");
	if (region == NULL)
	{
		printf("Error reading file!\n");
	}

	unsigned int offset[REGIONAREA];
	for (int i = 0; i < REGIONAREA; i++)
	{
		unsigned char buffer[4];
		fread(buffer, 1, 4, region);
		offset[i] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
	}

	for (int i = 0; i < REGIONAREA; i++)
	{
		if (offset[i] == 0) continue;

		printf("Reading from offset %d (at %d).\n", offset[i], offset[i] * SECTORSIZE);
		fseek(region, offset[i] * SECTORSIZE, SEEK_SET);
		unsigned char buffer[4];
		fread(buffer, 1, 4, region);
		int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

		printf("Reading %d bytes.\n", length);
		fseek(region, 1, SEEK_CUR);
		char* cdata = (char*)malloc(length);
		nbt_node* chunk = nbt_parse_compressed(cdata, length);
		if (errno != NBT_OK)
		{
			printf("Parsing error: %d\n", errno);
			return 0;
		}

		puts(nbt_dump_ascii(chunk));

		//render_chunk_colours(chunk, "regionchunk.png", "colours.csv");
		break;
	}

	return 0;
}
