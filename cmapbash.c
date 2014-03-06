#include <errno.h>

#include "nbt.h"

#include "region.h"
#include "render.h"


int main(int argc, char **argv)
{
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
		printf("Parsing error: %d\n", errno);
		return 0;
	}

	//puts(nbt_dump_ascii(chunk));

	render_chunk_heightmap(chunk, "heightmap.png");
	render_chunk_colours(chunk, "chunk.png", "colours.csv");

	nbt_free(chunk);
	*/

	render_region("r.0.0.mca");

	return 0;
}

