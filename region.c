#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "image.h"
#include "region.h"


void get_region_margins(const char* regionfile, int* margins, const char rotate)
{
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		return;
	}

	for (int i = 0; i < 4; i++) margins[i] = REGION_BLOCK_LENGTH;

	unsigned char buffer[4];
	unsigned int offset;
	for (int z = 0; z < REGION_BLOCK_LENGTH; z += CHUNK_BLOCK_LENGTH)
	{
		for (int x = 0; x < REGION_BLOCK_LENGTH; x += CHUNK_BLOCK_LENGTH)
		{
			fread(buffer, 1, 4, region);
			offset = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
			if (offset > 0)
			{
				// margins for this chunk, measured in pixels
				int cm[] = {
						z,
						REGION_BLOCK_LENGTH - x - CHUNK_BLOCK_LENGTH,
						REGION_BLOCK_LENGTH - z - CHUNK_BLOCK_LENGTH,
						x
				};

				// assign to rotated region margins if lower
				for (int i = 0; i < 4; i++)
				{
					if (cm[i] < margins[(i + rotate) % 4]) margins[(i + rotate) % 4] = cm[i];
				}
			}
		}
	}
	fclose(region);
}


void get_region_iso_margins(const char* regionfile, int* margins, const char rotate)
{
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		return;
	}

	// region margins measured in pixels
	// start at maximum and decrease as chunks are found
	margins[0] = margins[2] = ISO_REGION_TOP_HEIGHT; // top, bottom
	margins[1] = margins[3] = ISO_REGION_WIDTH; // right, left

	for (int cz = 0; cz < REGION_CHUNK_LENGTH; cz++)
	{
		for (int cx = 0; cx < REGION_CHUNK_LENGTH; cx++)
		{
			unsigned char buffer[4];
			fread(buffer, 1, 4, region);
			unsigned int offset = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
			if (offset > 0)
			{
				// margins for this chunk, measured in chunks
				int ccm[] = {
						REGION_CHUNK_LENGTH - cx - 1 + cz, // NE
						REGION_CHUNK_LENGTH - cx - 1 + REGION_CHUNK_LENGTH - cz - 1, // SE
						cx + REGION_CHUNK_LENGTH - cz - 1, // SW
						cx + cz // NW
				};

				for (int i = 0; i < 4; i++)
				{
					// margins for this chunk, measured in pixels
					// odd ones are multiplied by the horizontal margin, even ones by vertical
					int cmargin = ccm[i] *
							(i % 2 == rotate % 2 ? ISO_CHUNK_Y_MARGIN : ISO_CHUNK_X_MARGIN);

					// assign to rotated region margins if lower
					if (cmargin < margins[(i + rotate) % 4]) margins[(i + rotate) % 4] = cmargin;
				}
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


image render_region_blockmap(const char* regionfile, const texture* textures, const char night,
		const char rotate)
{
	image rimage;
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		rimage.data = NULL;
		return rimage;
	}

	rimage.width = REGION_BLOCK_LENGTH;
	rimage.height = REGION_BLOCK_LENGTH;
	rimage.data = (unsigned char*)calloc(REGION_BLOCK_AREA * CHANNELS, sizeof(char));

	unsigned int offsets[REGION_CHUNK_AREA];
	read_chunk_offsets(region, offsets);

	for (int cz = 0; cz < REGION_CHUNK_LENGTH; cz++)
	{
		for (int cx = 0; cx < REGION_CHUNK_LENGTH; cx++)
		{
			int c = get_rotated_index(cx, cz, REGION_CHUNK_LENGTH, rotate);
			if (offsets[c] == 0) continue;

			//printf("Reading chunk %d (%d, %d) from offset %d (at %#x).\n", c, cx, cz,
			//		offsets[c], offsets[c] * SECTOR_LENGTH);
			nbt_node* chunk = get_chunk_at_offset(region, offsets[c]);
			if (chunk == NULL) continue;

			image cimage = render_chunk_blockmap(chunk, textures, night, rotate);
			nbt_free(chunk);

			for (int bz = 0; bz < CHUNK_BLOCK_LENGTH; bz++)
			{
				// copy a line of pixel data from the chunk image to the region image
				int offset = (cz * CHUNK_BLOCK_LENGTH + bz) * rimage.width
						+ cx * CHUNK_BLOCK_LENGTH;
				memcpy(&rimage.data[offset * CHANNELS],
						&cimage.data[bz * CHUNK_BLOCK_LENGTH * CHANNELS],
						CHUNK_BLOCK_LENGTH * CHANNELS);
			}
			free(cimage.data);
		}
	}

	fclose(region);
	return rimage;
}


image render_region_iso_blockmap(const char* regionfile, const texture* textures, const char night,
		const char rotate)
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
	rimage.data = (unsigned char*)calloc(rimage.width * rimage.height * CHANNELS, sizeof(char));
	//printf("Rendering isometric image, %d x %d\n", rimage.width, rimage.height);

	for (int cz = 0; cz < REGION_CHUNK_LENGTH; cz++)
	{
		for (int cx = REGION_CHUNK_LENGTH - 1; cx >= 0; cx--)
		{
			int c = get_rotated_index(cx, cz, REGION_CHUNK_LENGTH, rotate);
			if (offsets[c] == 0) continue;

			//printf("Reading chunk %d (%d, %d) from offset %d (at %#x).\n", c, cx, cz,
			//		offsets[c], offsets[c] * SECTOR_LENGTH);
			nbt_node* chunk = get_chunk_at_offset(region, offsets[c]);
			if (chunk == NULL) continue;

			image cimage = render_chunk_iso_blockmap(chunk, textures, night, rotate);
			nbt_free(chunk);

			int cpx = (cx + cz) * ISO_CHUNK_X_MARGIN;
			int cpy = (REGION_CHUNK_LENGTH - cx + cz - 1) * ISO_CHUNK_Y_MARGIN;
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


void save_region_blockmap(const char* regionfile, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate)
{
	image rimage = isometric
			? render_region_iso_blockmap(regionfile, textures, night, rotate)
			: render_region_blockmap(regionfile, textures, night, rotate);
	if (rimage.data == NULL) return;

	printf("Saving image to %s ...\n", imagefile);
	SAVE_IMAGE(rimage, imagefile);
	free(rimage.data);
}
