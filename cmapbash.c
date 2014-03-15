#include <errno.h>
#include <stdlib.h>

#include "nbt.h"

//#include "world.h"
#include "isochunk.h"


int main(int argc, char **argv)
{
	int night = 0;
	char *outpath = "map.png";
	char *inpath = NULL;

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-n"))
		{
			printf("Night mode is on\n");
			night = 1;
		}
		else if (!strcmp(argv[i], "-o") && i + 1 < argc)
		{
			outpath = argv[i + 1];
			i++;
		}
		else if (inpath == NULL)
		{
			inpath = argv[i];
		}
	}

	if (inpath == NULL)
	{
		printf("Please specify a region directory.\n");
		return 0;
	}

	colour* colours = read_colours("colours.csv");

	//save_world_blockmap(regiondir, outfile, colours, night);

	nbt_node* chunk = nbt_parse_path(inpath);
	if (errno != NBT_OK)
	{
		printf("Error %d reading NBT file: %s\n", errno, inpath);
		return 0;
	}
	save_iso_chunk_blockmap(chunk, outpath, colours, night);
	nbt_free(chunk);

	free(colours);

	return 0;
}

