#include <stdio.h>
#include <stdlib.h>

#include "lodepng.h"

#define BLOCKTYPES 176
#define CHUNKWIDTH 16
#define CHUNKAREA CHUNKWIDTH * CHUNKWIDTH
#define SECTIONS 16
#define SECHEIGHT 16
#define SECSIZE SECHEIGHT * CHUNKAREA
#define CHUNKHEIGHT SECHEIGHT * SECTIONS


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
			char* value = strsep(&line, ",");
			unsigned char num = (value == NULL ? 0 : (char)strtol(value, NULL, 0));

			if (i == 0)
			{
				id = num;
				printf("id = %d\n", num);
			}
			else if (i == 1 && num != 0)
			{
				puts("Breaking");
				break;
			}
			else if (i > 1)
			{
				printf("%d %d\n", i, (int)num);
				//printf("setting %d (%d) to %d\n", id * 4 + i - 2, (int)&(colours[id * 4 + i - 2]), num);
				colours[id * 4 + i - 2] = num;
				//puts("done");
			}
		}
	}
	//free(line);
	return colours;
}


void render_blocks(const char* filename, const char* colourfile, const unsigned char* blocks,
		const unsigned w, const unsigned h)
{
	unsigned char* colours = read_colours(colourfile);
	unsigned char* image = (unsigned char*)malloc(w * h * 4);

	int offset;
	unsigned char blockid;
	for (int b = 0; b < w * h; b++)
	{
		for (int y = CHUNKHEIGHT - 1; y >= 0; y--)
		{
			blockid = blocks[y * CHUNKAREA + b];
			if (blockid != 0)
			{
				printf("Writing blockid %d at (%d, %d, %d)\n", (int)blockid,
						b % CHUNKWIDTH, b / CHUNKWIDTH, y);
				for (char c = 0; c < 4; c++)
				{
					image[b * 4 + c] = colours[blockid * 4 + c];
				}
				break;
			}
		}
	}
	lodepng_encode32_file("chunk.png", image, w, h);
	free(colours);
	free(image);
}


void render_greyscale(const char* filename, const unsigned char* values,
		const unsigned w, const unsigned h)
{
	unsigned char* image = (unsigned char*)malloc(w * h * 4);
	for (int b = 0; b < w * h; b++)
	{
		// colour values must be big-endian, so copy them manually
		image[b * 4] = values[b];
		image[b * 4 + 1] = values[b];
		image[b * 4 + 2] = values[b];
		image[b * 4 + 3] = 255;
	}
	lodepng_encode32_file("chunk.png", image, w, h);
	free(image);
}
