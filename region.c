#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "region.h"


void get_region_margins(const char* regionfile, int* margins)
{
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		return;
	}

	for (int i = 0; i < 4; i++)
	{
		margins[i] = REGION_BLOCK_WIDTH;
	}

	unsigned char buffer[4];
	unsigned int offset;
	for (int z = 0; z < REGION_BLOCK_WIDTH; z += CHUNK_BLOCK_WIDTH)
	{
		for (int x = 0; x < REGION_BLOCK_WIDTH; x += CHUNK_BLOCK_WIDTH)
		{
			fread(buffer, 1, 4, region);
			offset = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
			if (offset > 0)
			{
				margins[0] = z < margins[0] ? z : margins[0]; // north
				margins[1] = (REGION_BLOCK_WIDTH - x - CHUNK_BLOCK_WIDTH) < margins[1] ?
						(REGION_BLOCK_WIDTH - x - CHUNK_BLOCK_WIDTH) : margins[1]; // east
				margins[2] = (REGION_BLOCK_WIDTH - z - CHUNK_BLOCK_WIDTH) < margins[2] ?
						(REGION_BLOCK_WIDTH - z - CHUNK_BLOCK_WIDTH) : margins[2]; // south
				margins[3] = x < margins[3] ? x : margins[3]; // west
			}
		}
	}
	fclose(region);
}


unsigned char* render_region_blockmap(const char* regionfile, const colour* colours,
		const char night)
{
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
	}

	unsigned int offset[REGION_CHUNK_AREA];
	for (int c = 0; c < REGION_CHUNK_AREA; c++)
	{
		unsigned char buffer[4];
		fread(buffer, 1, 4, region);
		offset[c] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
	}

	unsigned char* regionimage = (unsigned char*)calloc(REGION_BLOCK_AREA * 4, sizeof(char));

	for (int cz = 0; cz < REGION_CHUNK_WIDTH; cz++)
	{
		for (int cx = 0; cx < REGION_CHUNK_WIDTH; cx++)
		{
			int c = cz * REGION_CHUNK_WIDTH + cx;
			if (offset[c] == 0) continue;

			//printf("Reading chunk %d (%d, %d) from offset %d (at %#x).\n", c, cx, cz,
			//		offset[c], offset[c] * SECTOR_LENGTH);
			fseek(region, offset[c] * SECTOR_LENGTH, SEEK_SET);
			unsigned char buffer[4];
			fread(buffer, 1, 4, region);
			int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

			fseek(region, 1, SEEK_CUR);
			//printf("Reading %d bytes at %#lx.\n", length, ftell(region));
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

			unsigned char* chunkimage = render_chunk_blockmap(chunk, colours, night);
			nbt_free(chunk);

			for (int bz = 0; bz < CHUNK_BLOCK_WIDTH; bz++)
			{
				// copy a line of pixel data from the chunk image to the region image
				int offset = (cz * CHUNK_BLOCK_WIDTH + bz) * REGION_BLOCK_WIDTH
						+ cx * CHUNK_BLOCK_WIDTH;
				memcpy(&regionimage[offset * CHANNELS],
						&chunkimage[bz * CHUNK_BLOCK_WIDTH * CHANNELS],
						CHUNK_BLOCK_WIDTH * CHANNELS);
			}
			free(chunkimage);
		}
	}

	fclose(region);
	return regionimage;
}


void save_region_blockmap(const char* regionfile, const char* imagefile, const colour* colours,
		const char night)
{
	unsigned char* regionimage = render_region_blockmap(regionfile, colours, night);
	lodepng_encode32_file(imagefile, regionimage, REGION_BLOCK_WIDTH, REGION_BLOCK_WIDTH);
	free(regionimage);
}
