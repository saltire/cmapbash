#include <errno.h>

#include "nbt.h"

#include "lodepng.h"

#include "render.h"


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

	render_chunk_heightmap(chunk, "heightmap.png");
	render_chunk_colours(chunk, "chunk.png", "colours.csv");

	nbt_free(chunk);
	return 0;
}

