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


image render_region_blockmap(const char* regionfile, const texture* textures, const char night,
		const char rotate, char* neighbourfiles[4])
{
	image rimage = create_image(REGION_BLOCK_LENGTH, REGION_BLOCK_LENGTH);

	region reg = read_region(regionfile);
	if (reg.file == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		rimage.data = NULL;
		return rimage;
	}

	region nregions[4];
	for (int i = 0; i < 4; i++)
	{
		nregions[i] = read_region(
				(neighbourfiles == NULL || neighbourfiles[i] == NULL) ? "" : neighbourfiles[i]);
	}

	for (int cz = 0; cz <= MAX_CHUNK; cz++)
	{
		for (int cx = 0; cx <= MAX_CHUNK; cx++)
		{
			nbt_node* chunk = get_chunk(reg, cx, cz, rotate);
			if (chunk == NULL) continue;

			nbt_node* nchunks[4] = {
					cz > 0 ? get_chunk(reg, cx, cz - 1, rotate) :
							(!nregions[0].loaded ? NULL :
									get_chunk(nregions[0], cx, MAX_CHUNK, rotate)),

					cx < MAX_CHUNK ? get_chunk(reg, cx + 1, cz, rotate) :
							(!nregions[1].loaded ? NULL :
									get_chunk(nregions[1], 0, cz, rotate)),

					cz < MAX_CHUNK ? get_chunk(reg, cx, cz + 1, rotate) :
							(!nregions[2].loaded ? NULL :
									get_chunk(nregions[2], cx, 0, rotate)),

					cx > 0 ? get_chunk(reg, cx - 1, cz, rotate) :
							(!nregions[3].loaded ? NULL :
									get_chunk(nregions[3], MAX_CHUNK, cz, rotate))
			};

			image cimage = render_chunk_blockmap(chunk, textures, night, rotate, nchunks);
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
			free_image(cimage);
		}
	}

	fclose(reg.file);
	return rimage;
}


image render_region_iso_blockmap(const char* regionfile, const texture* textures, const char night,
		const char rotate, char* neighbourfiles[4])
{
	image rimage = create_image(ISO_REGION_WIDTH, ISO_REGION_HEIGHT);

	region reg = read_region(regionfile);
	if (reg.file == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
		rimage.data = NULL;
		return rimage;
	}
	//printf("Rendering isometric image, %d x %d\n", rimage.width, rimage.height);

	region nregions[4];
	for (int i = 0; i < 4; i++)
	{
		nregions[i] = read_region(
				(neighbourfiles == NULL || neighbourfiles[i] == NULL) ? "" : neighbourfiles[i]);
	}

	for (int cz = 0; cz <= MAX_CHUNK; cz++)
	{
		for (int cx = 0; cx <= MAX_CHUNK; cx++)
		{
			nbt_node* chunk = get_chunk(reg, cx, cz, rotate);
			if (chunk == NULL) continue;

			nbt_node* nchunks[4] = {
					cz > 0 ? get_chunk(reg, cx, cz - 1, rotate) :
							(!nregions[0].loaded ? NULL :
									get_chunk(nregions[0], cx, MAX_CHUNK, rotate)),

					cx < MAX_CHUNK ? get_chunk(reg, cx + 1, cz, rotate) :
							(!nregions[1].loaded ? NULL :
									get_chunk(nregions[1], 0, cz, rotate)),

					cz < MAX_CHUNK ? get_chunk(reg, cx, cz + 1, rotate) :
							(!nregions[2].loaded ? NULL :
									get_chunk(nregions[2], cx, 0, rotate)),

					cx > 0 ? get_chunk(reg, cx - 1, cz, rotate) :
							(!nregions[3].loaded ? NULL :
									get_chunk(nregions[3], MAX_CHUNK, cz, rotate))
			};

			image cimage = render_chunk_iso_blockmap(chunk, textures, night, rotate, nchunks);
			nbt_free(chunk);

			// translate orthographic to isometric coordinates
			int cpx = (cx + MAX_CHUNK - cz) * ISO_CHUNK_X_MARGIN;
			int cpy = (cx + cz) * ISO_CHUNK_Y_MARGIN;
			//printf("Writing chunk %d,%d to pixel %d,%d\n", cx, cz, cpx, cpy);

			for (int py = 0; py < ISO_CHUNK_HEIGHT; py++)
			{
				for (int px = 0; px < ISO_CHUNK_WIDTH; px++)
				{
					combine_alpha(&cimage.data[(py * ISO_CHUNK_WIDTH + px) * CHANNELS],
							&rimage.data[((cpy + py) * rimage.width + cpx + px) * CHANNELS], 1);
				}
			}
			free_image(cimage);
		}
	}

	fclose(reg.file);
	return rimage;
}


void save_region_blockmap(const char* regionfile, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate)
{
	image rimage = isometric
			? render_region_iso_blockmap(regionfile, textures, night, rotate, NULL)
			: render_region_blockmap(regionfile, textures, night, rotate, NULL);
	if (rimage.data == NULL) return;

	printf("Saving image to %s ...\n", imagefile);
	save_image(rimage, imagefile);
	free_image(rimage);
}
