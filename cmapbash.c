#include <errno.h>
#include <stdlib.h>

#include "nbt.h"

#include "lodepng.h"

#include "chunk.h"
#include "render.h"

#define BLOCKS 16
#define CHUNKSIZE BLOCKS * BLOCKS


int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("Please specify an NBT file to parse.\n");
		return 0;
	}

	printf("Reading file %s\n", argv[1]);
	nbt_node* chunk = nbt_parse_path(argv[1]);
	if (errno != NBT_OK)
	{
		printf("Parsing error!\n");
		return 0;
	}

	//puts(nbt_dump_ascii(chunk));

	/*
	unsigned char* heightmap = get_chunk_heightmap(chunk);
	render_greyscale("chunk.png", heightmap, BLOCKS, BLOCKS);
	free(heightmap);
	*/

	unsigned char* blocks = get_chunk_blocks(chunk);
	render_blocks("chunk.png", "colours.csv", blocks, BLOCKS, BLOCKS);
	free(blocks);

	nbt_free(chunk);
	return 0;
}

