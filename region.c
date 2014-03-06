#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "nbt.h"

#include "lodepng.h"

#include "region.h"


#define SECTOR_LENGTH 4096


unsigned char* render_region_blockmap(const char* regionfile, const char* colourfile)
{
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error reading file!\n");
	}

	unsigned int offset[REGION_CHUNK_AREA];
	for (int c = 0; c < REGION_CHUNK_AREA; c++)
	{
		unsigned char buffer[4];
		fread(buffer, 1, 4, region);
		offset[c] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
	}

	unsigned char* regionimage = (unsigned char*)calloc(REGION_BLOCK_AREA * 4, sizeof(char));

	unsigned int cx, cz, c, bx, bz, b, px, pz, p;
	for (cz = 0; cz < REGION_CHUNK_WIDTH; cz++)
	{
		for (cx = 0; cx < REGION_CHUNK_WIDTH; cx++)
		{
			c = cz * REGION_CHUNK_WIDTH + cx;
			if (offset[c] == 0) continue;

			printf("Reading chunk %d (%d, %d) from offset %d (at %d).\n", c, cx, cz,
					offset[c], offset[c] * SECTOR_LENGTH);
			fseek(region, offset[c] * SECTOR_LENGTH, SEEK_SET);
			unsigned char buffer[4];
			fread(buffer, 1, 4, region);
			int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

			fseek(region, 1, SEEK_CUR);
			//printf("Reading %d bytes at %ld.\n", length, ftell(region));
			char* cdata = (char*)malloc(length);
			fread(cdata, length, 1, region);
			nbt_node* chunk = nbt_parse_compressed(cdata, length);
			if (errno != NBT_OK)
			{
				printf("Parsing error: %d\n", errno);
				return 0;
			}
			free(cdata);

			//puts(nbt_dump_ascii(chunk));

			unsigned char* chunkimage = render_chunk_blockmap(chunk, colourfile);
			free(chunk);

			for (bz = 0; bz < CHUNK_BLOCK_WIDTH; bz++)
			{
				for (bx = 0; bx < CHUNK_BLOCK_WIDTH; bx++)
				{
					// the position of the block in the chunk
					b = bz * CHUNK_BLOCK_WIDTH + bx;

					// the position of the block in the region (pixel #)
					px = cx * CHUNK_BLOCK_WIDTH + bx;
					pz = cz * CHUNK_BLOCK_WIDTH + bz;
					p = pz * REGION_BLOCK_WIDTH + px;

					// add each channel in big-endian order
					for (char ch = 0; ch < 4; ch++)
					{
						regionimage[p * 4 + ch] = chunkimage[b * 4 + ch];
					}
				}
			}

			// [paste chunk image into region image]
			free(chunkimage);
		}
	}
	return regionimage;
}


void save_region_blockmap(const char* regionfile, const char* imagefile, const char* colourfile)
{
	unsigned char* regionimage = render_region_blockmap(regionfile, colourfile);
	lodepng_encode32_file(imagefile, regionimage, REGION_BLOCK_WIDTH, REGION_BLOCK_WIDTH);
	free(regionimage);
}
