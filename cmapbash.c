#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "textures.h"
#include "world.h"


int main(int argc, char **argv)
{
	unsigned char isometric = 0;
	unsigned char night = 0;
	unsigned char rotate = 0;
	char *outpath = "map.png";
	char *inpath = NULL;

	unsigned int rotateint;

	for (int i = 1; i < argc; i++)
	{
		// isometric mode
		if (!strcmp(argv[i], "-i"))
		{
			printf("Isometric mode is on\n");
			isometric = 1;
		}
		// night mode
		else if (!strcmp(argv[i], "-n"))
		{
			printf("Night mode is on\n");
			night = 1;
		}
		// rotation
		else if (!strcmp(argv[i], "-r") && i + 1 < argc)
		{
			unsigned int rotateint;
			if (sscanf(argv[i + 1], "%d", &rotateint))
			{
				rotate = (unsigned char)rotateint % 4;
				printf("Rotation is %d\n", rotate);
			}
			else
			{
				printf("Invalid rotate value: %s\n", argv[i + 1]);
			}
			i++;
		}
		// image save path
		else if (!strcmp(argv[i], "-o") && i + 1 < argc)
		{
			outpath = argv[i + 1];
			i++;
		}
		// source data path
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

	textures* tex = read_textures("textures.csv", "shapes.csv");

//	nbt_node* chunk = nbt_parse_path(inpath);
//	if (chunk == NULL)
//	{
//		printf("Error reading chunk file\n");
//		return 0;
//	}
//	save_chunk_map(chunk, outpath, tex, night, isometric, rotate);

	//save_region_map(inpath, outpath, tex, night, isometric, rotate);

	save_world_map(inpath, outpath, tex, night, isometric, rotate);

	free_textures(tex);

	return 0;
}

