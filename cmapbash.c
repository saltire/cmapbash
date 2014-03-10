#include <errno.h>
#include <stdlib.h>

#include "nbt.h"

#include "colours.h"
#include "chunk.h"
#include "region.h"
#include "world.h"


int main(int argc, char **argv)
{
	unsigned char* colours = read_colours("colours.csv");
/*
	if (argc < 2)
	{
		printf("Please specify an NBT file to parse.\n");
		return 0;
	}

	printf("Reading file %s\n", argv[1]);
	nbt_node* chunk = nbt_parse_path(argv[1]);
	if (errno != NBT_OK)
	{
		printf("Error %d reading NBT file: %s\n", errno, argv[1]);
		return 0;
	}

	puts(nbt_dump_ascii(chunk));

	//save_chunk_heightmap(chunk, "heightmap.png");
	save_chunk_blockmap(chunk, "chunk.png", colours);

	nbt_free(chunk);
*/

/*
	if (argc < 2)
	{
		printf("Please specify a region file to parse.\n");
		return 0;
	}

	save_region_blockmap(argv[1], "regionmap.png", colours, 1);
*/


	if (argc < 2)
	{
		printf("Please specify a world directory.\n");
		return 0;
	}
	save_world_blockmap(argv[1], "worldmap.png", colours, 1);

	free(colours);
	return 0;
}

