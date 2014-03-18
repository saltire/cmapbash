#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "lodepng.h"

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


static void read_chunk_offsets(FILE* region, unsigned int* offsets)
{
	for (int c = 0; c < REGION_CHUNK_AREA; c++)
	{
		unsigned char buffer[4];
		fread(buffer, 1, 4, region);
		offsets[c] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
	}
}


static nbt_node* get_chunk_at_offset(FILE* region, unsigned int offset)
{
	fseek(region, offset * SECTOR_LENGTH, SEEK_SET);
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
		return NULL;
	}
	free(cdata);

	return chunk;
}


image render_region_blockmap(const char* regionfile, const colour* colours,
		const char night)
{
	image rimage;
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		rimage.data = NULL;
		return rimage;
	}

	rimage.width = REGION_BLOCK_WIDTH;
	rimage.height = REGION_BLOCK_WIDTH;
	rimage.data = (unsigned char*)calloc(REGION_BLOCK_AREA * CHANNELS, sizeof(char));
	//printf("Rendering isometric image, %d x %d\n", rimage.width, rimage.height);


	unsigned int offsets[REGION_CHUNK_AREA];
	read_chunk_offsets(region, offsets);

	for (int cz = 0; cz < REGION_CHUNK_WIDTH; cz++)
	{
		for (int cx = 0; cx < REGION_CHUNK_WIDTH; cx++)
		{
			int c = cz * REGION_CHUNK_WIDTH + cx;
			if (offsets[c] == 0) continue;

			//printf("Reading chunk %d (%d, %d) from offset %d (at %#x).\n", c, cx, cz,
			//		offsets[c], offsets[c] * SECTOR_LENGTH);
			nbt_node* chunk = get_chunk_at_offset(region, offsets[c]);
			if (chunk == NULL) continue;

			image cimage = render_chunk_blockmap(chunk, colours, night);
			nbt_free(chunk);

			for (int bz = 0; bz < CHUNK_BLOCK_WIDTH; bz++)
			{
				// copy a line of pixel data from the chunk image to the region image
				int offset = (cz * CHUNK_BLOCK_WIDTH + bz) * rimage.width
						+ cx * CHUNK_BLOCK_WIDTH;
				memcpy(&rimage.data[offset * CHANNELS],
						&cimage.data[bz * CHUNK_BLOCK_WIDTH * CHANNELS],
						CHUNK_BLOCK_WIDTH * CHANNELS);
			}
			free(cimage.data);
		}
	}

	fclose(region);
	return rimage;
}


image render_region_iso_blockmap(const char* regionfile, const colour* colours,
		const char night)
{
	image rimage;

	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		rimage.data = NULL;
		return rimage;
	}

	unsigned int offsets[REGION_CHUNK_AREA];
	read_chunk_offsets(region, offsets);

	rimage.width = ISO_REGION_WIDTH;
	rimage.height = ISO_REGION_HEIGHT;
	rimage.data = (unsigned char*)calloc(
			ISO_REGION_WIDTH * ISO_REGION_HEIGHT * CHANNELS, sizeof(char));
	//printf("Rendering isometric image, %d x %d\n", rimage.width, rimage.height);

	for (int cz = 0; cz < REGION_CHUNK_WIDTH; cz++)
	{
		for (int cx = REGION_CHUNK_WIDTH - 1; cx >= 0; cx--)
		{
			int c = cz * REGION_CHUNK_WIDTH + cx;
			if (offsets[c] == 0) continue;

			//printf("Reading chunk %d (%d, %d) from offset %d (at %#x).\n", c, cx, cz,
			//		offsets[c], offsets[c] * SECTOR_LENGTH);
			nbt_node* chunk = get_chunk_at_offset(region, offsets[c]);
			if (chunk == NULL) continue;

			image cimage = render_chunk_iso_blockmap(chunk, colours, night);
			nbt_free(chunk);

			int cpx = (cx + cz) * ISO_CHUNK_WIDTH / 2;
			int cpy = (REGION_CHUNK_WIDTH - cx + cz - 1) * CHUNK_BLOCK_WIDTH * ISO_BLOCK_STEP;
			//printf("Writing chunk %d,%d to pixel %d,%d\n", cx, cz, cpx, cpy);

			for (int py = 0; py < ISO_CHUNK_HEIGHT; py++)
			{
				for (int px = 0; px < ISO_CHUNK_WIDTH; px++)
				{
					combine_alpha(&cimage.data[(py * ISO_CHUNK_WIDTH + px) * CHANNELS],
							&rimage.data[((cpy + py) * rimage.width + cpx + px) * CHANNELS], 1);
				}
			}
			free(cimage.data);
		}
	}

	fclose(region);
	return rimage;
}


void save_region_blockmap(const char* regionfile, const char* imagefile, const colour* colours,
		const char night, const char isometric)
{
	image rimage = isometric
			? render_region_iso_blockmap(regionfile, colours, night)
			: render_region_blockmap(regionfile, colours, night);
	if (rimage.data == NULL) return;

	printf("Saving image to %s ...\n", imagefile);
	lodepng_encode32_file(imagefile, rimage.data, rimage.width, rimage.height);
	free(rimage.data);
}
