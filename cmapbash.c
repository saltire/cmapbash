#include <errno.h>
#include <stdlib.h>

#include "nbt.h"

#include "world.h"


int main(int argc, char **argv)
{
	int night = 0;
	char *outfile = "map.png";
	char *regiondir = NULL;

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-n"))
		{
			printf("Night mode is on\n");
			night = 1;
		}
		else if (!strcmp(argv[i], "-o") && i + 1 < argc)
		{
			outfile = argv[i + 1];
			i++;
		}
		else if (regiondir == NULL)
		{
			regiondir = argv[i];
		}
	}

	if (regiondir == NULL)
	{
		printf("Please specify a region directory.\n");
		return 0;
	}

	colour* colours = read_colours("colours.csv");
	save_world_blockmap(regiondir, outfile, colours, night);
	free(colours);

	return 0;
}

