#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "region.h"


#define MAX_CHUNK REGION_CHUNK_LENGTH - 1
#define SECTOR_LENGTH 4096


typedef struct region {
	FILE* file;
	unsigned int offsets[REGION_CHUNK_AREA];
	unsigned char loaded;
} region;


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
	FILE* rfile = fopen(regionfile, "r");
	if (rfile == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		return;
	}

	// region margins measured in pixels
	// start at maximum and decrease as chunks are found
	margins[0] = margins[2] = ISO_REGION_TOP_HEIGHT; // top, bottom
	margins[1] = margins[3] = ISO_REGION_WIDTH; // right, left

	for (int cz = 0; cz <= MAX_CHUNK; cz++)
	{
		for (int cx = 0; cx <= MAX_CHUNK; cx++)
		{
			unsigned char buffer[4];
			fread(buffer, 1, 4, rfile);
			unsigned int offset = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
			if (offset > 0)
			{
				// margins for this chunk, measured in chunks
				int ccm[] = {
						cx + cz, // NW
						MAX_CHUNK - cx + cz, // NE
						MAX_CHUNK - cx + MAX_CHUNK - cz, // SE
						cx + MAX_CHUNK - cz, // SW
				};

				for (int i = 0; i < 4; i++)
				{
					// margins for this chunk, measured in pixels
					// multiplied by the horizontal or vertical margin depending on rotation
					int cmargin = ccm[i] *
							(i % 2 == rotate % 2 ? ISO_CHUNK_Y_MARGIN : ISO_CHUNK_X_MARGIN);

					// assign to rotated region margins if lower
					if (cmargin < margins[(i + rotate) % 4]) margins[(i + rotate) % 4] = cmargin;
				}
			}
		}
	}
	fclose(rfile);
}


static region read_region(const char* regionfile)
{
	region reg;
	reg.file = fopen(regionfile, "r");
	if (reg.file == NULL) reg.loaded = 0;
	else
	{
		for (int c = 0; c < REGION_CHUNK_AREA; c++)
		{
			unsigned char buffer[4];
			fread(buffer, 1, 4, reg.file);
			reg.offsets[c] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
		}
		reg.loaded = 1;
	}
	return reg;
}


static nbt_node* get_chunk(region reg, int cx, int cz, const char rotate)
{
	// get absolute chunk coordinates from rotated chunk coordinates
	int x, z;
	switch(rotate) {
	case 0:
		x = cx;
		z = cz;
		break;
	case 1:
		x = cz;
		z = MAX_CHUNK - cx;
		break;
	case 2:
		x = MAX_CHUNK - cx;
		z = MAX_CHUNK - cz;
		break;
	case 3:
		x = MAX_CHUNK - cz;
		z = cx;
		break;
	}

	int offset = reg.offsets[z * REGION_CHUNK_LENGTH + x];
	if (offset == 0) return NULL;

	unsigned char buffer[4];
	fseek(reg.file, offset * SECTOR_LENGTH, SEEK_SET);
	fread(buffer, 1, 4, reg.file);
	int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

	char* cdata = (char*)malloc(length);
	fseek(reg.file, 1, SEEK_CUR);
	//printf("Reading %d bytes at %#lx.\n", length, ftell(reg.file));
	fread(cdata, length, 1, reg.file);

	nbt_node* chunk = nbt_parse_compressed(cdata, length);
	if (errno != NBT_OK)
	{
		printf("Parsing error: %d\n", errno);
		return NULL;
	}
	free(cdata);

	return chunk;
}


void render_region_map(image* image, const int rpx, const int rpy,
		const char* regionfile, char* nfiles[4], const textures* textures,
		const char night, const char isometric, const char rotate)
{
	region reg = read_region(regionfile);
	if (reg.file == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		return;
	}

	region nregions[4];
	for (int i = 0; i < 4; i++)
		nregions[i] = read_region((nfiles == NULL || nfiles[i] == NULL) ? "" : nfiles[i]);

	nbt_node *chunk, *prev_chunk, *next_chunk, *nchunks[4];
	for (int cz = 0; cz <= MAX_CHUNK; cz++)
	{
		for (int cx = 0; cx <= MAX_CHUNK; cx++)
		{
			// get this chunk
			chunk = cx > 0 && prev_chunk != NULL ? next_chunk : get_chunk(reg, cx, cz, rotate);
			if (chunk == NULL)
			{
				if (cx > 0) nbt_free(prev_chunk);
				prev_chunk = NULL;
				continue;
			}

			// get neighbouring chunks, either from this region or a neighbouring one
			nchunks[0] = cz > 0 ? get_chunk(reg, cx, cz - 1, rotate) :
					(!nregions[0].loaded ? NULL : get_chunk(nregions[0], cx, MAX_CHUNK, rotate));

			nchunks[1] = cx < MAX_CHUNK ? get_chunk(reg, cx + 1, cz, rotate) :
					(!nregions[1].loaded ? NULL : get_chunk(nregions[1], 0, cz, rotate));

			nchunks[2] = cz < MAX_CHUNK ? get_chunk(reg, cx, cz + 1, rotate) :
					(!nregions[2].loaded ? NULL : get_chunk(nregions[2], cx, 0, rotate));

			nchunks[3] = cx > 0 ? prev_chunk :
					(!nregions[3].loaded ? NULL : get_chunk(nregions[3], MAX_CHUNK, cz, rotate));

			// render chunk image onto region image
			int cpx, cpy;
			if (isometric)
			{
				// translate orthographic to isometric coordinates
				cpx = rpx + (cx + MAX_CHUNK - cz) * ISO_CHUNK_X_MARGIN;
				cpy = rpy + (cx + cz) * ISO_CHUNK_Y_MARGIN;
			}
			else
			{
				cpx = rpx + cx * CHUNK_BLOCK_LENGTH;
				cpy = rpy + cz * CHUNK_BLOCK_LENGTH;
			}
			render_chunk_map(image, cpx, cpy, chunk, nchunks, textures, night, isometric, rotate);

			// free chunks, or pass them to the next iteration
			nbt_free(nchunks[0]);
			nbt_free(nchunks[2]);
			nbt_free(nchunks[3]);
			if (cx == MAX_CHUNK)
			{
				nbt_free(chunk);
				nbt_free(nchunks[1]);
			}
			else
			{
				prev_chunk = chunk;
				next_chunk = nchunks[1];
			}
		}
	}

	fclose(reg.file);
}


void save_region_map(const char* regionfile, const char* imagefile, const textures* textures,
		const char night, const char isometric, const char rotate)
{
	image rimage = isometric ?
			create_image(ISO_REGION_WIDTH, ISO_REGION_HEIGHT) :
			create_image(REGION_BLOCK_LENGTH, REGION_BLOCK_LENGTH);

	render_region_map(&rimage, 0, 0, regionfile, NULL, textures, night, isometric, rotate);
	if (rimage.data == NULL) return;

	printf("Saving image to %s ...\n", imagefile);
	save_image(rimage, imagefile);
	free_image(rimage);
}
