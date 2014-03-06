#include <stdio.h>
#include <stdlib.h>

#include "nbt.h"

#include "lodepng.h"

#include "chunk.h"


#define BLOCKTYPES 176


void render_chunk_heightmap(nbt_node* chunk, const char* filename)
{
	unsigned char* heightmap = get_chunk_heightmap(chunk);
	unsigned char* image = (unsigned char*)malloc(CHUNKAREA * 4);
	for (int b = 0; b < CHUNKAREA; b++)
	{
		//printf("Rendering height %d at (%d, %d)\n",
		//		(int)heightmap[b], b % CHUNKWIDTH, b / CHUNKWIDTH);

		// colour values must be big-endian, so copy them manually
		image[b * 4] = heightmap[b];
		image[b * 4 + 1] = heightmap[b];
		image[b * 4 + 2] = heightmap[b];
		image[b * 4 + 3] = 255;
	}
	lodepng_encode32_file(filename, image, CHUNKWIDTH, CHUNKWIDTH);
	free(image);
}


unsigned char* read_colours(const char* filename)
{
	FILE* csv = fopen(filename, "r");
	if (csv == NULL)
	{
		printf("Error reading file!\n");
	}

	unsigned char* colours = (unsigned char*)malloc(BLOCKTYPES * 4);

	char* line = (char*)malloc(60);
	while (fgets(line, 60, csv))
	{
		unsigned char id;
		for (int i = 0; i < 6; i++)
		{
			char* value = strtok(i == 0 ? line : NULL, ",");
			unsigned char num = (value == NULL ? 0 : (char)strtol(value, NULL, 0));

			if (i == 0)
			{
				id = num;
				//printf("id = %d\n", num);
			}
			else if (i == 1 && num != 0)
			{
				//printf("Data type = %d; breaking\n", (int)num);
				break;
			}
			else if (i > 1)
			{
				//printf("setting %d (%d) to %d\n", id * 4 + i - 2, (int)&(colours[id * 4 + i - 2]), num);
				colours[id * 4 + i - 2] = num;
				//puts("done");
			}
		}
	}
	//free(line);
	return colours;
}


void render_chunk_colours(nbt_node* chunk, const char* filename, const char* colourfile)
{
	unsigned char* blocks = get_chunk_blocks(chunk);
	unsigned char* colours = read_colours(colourfile);
	unsigned char* image = (unsigned char*)malloc(CHUNKAREA * 4);

	int offset;
	unsigned char blockid;
	for (int b = 0; b < CHUNKAREA; b++)
	{
		for (int y = CHUNKHEIGHT - 1; y >= 0; y--)
		{
			blockid = blocks[y * CHUNKAREA + b];
			if (blockid != 0)
			{
				//printf("Writing blockid %d at (%d, %d, %d)\n", (int)blockid,
				//		b % CHUNKWIDTH, b / CHUNKWIDTH, y);

				// colour values must be big-endian, so copy them manually
				for (char c = 0; c < 4; c++)
				{
					image[b * 4 + c] = colours[blockid * 4 + c];
				}
				break;
			}
		}
	}
	lodepng_encode32_file(filename, image, CHUNKWIDTH, CHUNKWIDTH);
	free(blocks);
	free(colours);
	free(image);
}
