#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "nbt.h"

#include "colours.h"
#include "region.h"


int main(int argc, char **argv)
{
	int isometric = 0;
	int night = 0;
	char *outpath = "map.png";
	char *inpath = NULL;

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i"))
		{
			printf("Isometric mode is on\n");
			isometric = 1;
		}
		else if (!strcmp(argv[i], "-n"))
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

	save_region_blockmap(inpath, outpath, colours, night, isometric);

	free(colours);

	return 0;
}

