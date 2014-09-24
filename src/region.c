/*
	cmapbash - a simple Minecraft map renderer written in C.
	© 2014 saltire sable, x@saltiresable.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chunk.h"
#include "dims.h"
#include "image.h"
#include "region.h"


#define MAX_CHUNK REGION_CHUNK_LENGTH - 1
#define SECTOR_LENGTH 4096


typedef struct region
{
	FILE *file;
	unsigned int offsets[REGION_CHUNK_AREA];
	unsigned char loaded;
}
region;


void get_region_margins(const char *regionfile, int *margins, const char rotate)
{
	FILE *region = fopen(regionfile, "rb");
	if (region == NULL)
	{
		fprintf(stderr, "Error %d reading region file: %s\n", errno, regionfile);
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
				int cm[] =
				{
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


void get_region_iso_margins(const char *regionfile, int *margins, const char rotate)
{
	FILE *rfile = fopen(regionfile, "rb");
	if (rfile == NULL)
	{
		fprintf(stderr, "Error %d reading region file: %s\n", errno, regionfile);
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
				int ccm[] =
				{
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


static region read_region(const char *regionfile)
{
	region reg;
	reg.file = fopen(regionfile, "rb");
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


static int get_chunk_offset(region reg, int rcx, int rcz, const char rotate)
{
	// get absolute chunk coordinates from rotated chunk coordinates
	int cx, cz;
	switch(rotate) {
	case 0:
		cx = rcx;
		cz = rcz;
		break;
	case 1:
		cx = rcz;
		cz = MAX_CHUNK - rcx;
		break;
	case 2:
		cx = MAX_CHUNK - rcx;
		cz = MAX_CHUNK - rcz;
		break;
	case 3:
		cx = MAX_CHUNK - rcz;
		cz = rcx;
		break;
	}

	return reg.offsets[cz * REGION_CHUNK_LENGTH + cx];
}


static nbt_node *get_chunk(region reg, int rcx, int rcz, const char rotate)
{
	int offset = get_chunk_offset(reg, rcx, rcz, rotate);
	if (offset == 0) return NULL;

	unsigned char buffer[4];
	fseek(reg.file, offset * SECTOR_LENGTH, SEEK_SET);
	fread(buffer, 1, 4, reg.file);
	int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

	char *cdata = (char*)malloc(length);
	fseek(reg.file, 1, SEEK_CUR);
	//printf("Reading %d bytes at %#lx.\n", length, ftell(reg.file));
	fread(cdata, length, 1, reg.file);

	nbt_node *chunk_nbt = nbt_parse_compressed(cdata, length);
	if (errno != NBT_OK)
	{
		fprintf(stderr, "Error %d parsing chunk\n", errno);
		return NULL;
	}
	free(cdata);

	return chunk_nbt;
}


void render_tiny_region_map(image *image, const int rpx, const int rpy, const char *regionfile,
		const options opts)
{
	region reg = read_region(regionfile);
	if (reg.file == NULL)
	{
		fprintf(stderr, "Error %d reading region file: %s\n", errno, regionfile);
		return;
	}

	unsigned char colour[CHANNELS] = {255, 255, 255, 255};

	for (int rcz = 0; rcz <= MAX_CHUNK; rcz++)
	{
		for (int rcx = 0; rcx <= MAX_CHUNK; rcx++)
		{
			// check for the chunk's existence using its rotated coordinates
			int offset = get_chunk_offset(reg, rcx, rcz, opts.rotate);
			if (offset == 0) continue;

			// if it exists, colour this pixel
			int cpx = rpx + rcx;
			int cpy = rpy + rcz;
			unsigned char *pixel = &image->data[(cpy * image->width + cpx) * CHANNELS];
			memcpy(pixel, colour, CHANNELS);
		}
	}

	fclose(reg.file);
}


void render_region_map(image *image, const int rpx, const int rpy, const char *regionfile,
		char *nfiles[4], const textures *tex, const options opts)
{
	region reg = read_region(regionfile);
	if (reg.file == NULL)
	{
		fprintf(stderr, "Error %d reading region file: %s\n", errno, regionfile);
		return;
	}

	region nregions[4];
	for (int i = 0; i < 4; i++)
		nregions[i] = read_region((nfiles == NULL || nfiles[i] == NULL) ? "" : nfiles[i]);

	nbt_node *chunk, *prev_chunk, *new_chunk, *nchunks[4];
	// use rotated chunk coordinates, since we need to draw them top to bottom for isometric
	for (int rcz = 0; rcz <= MAX_CHUNK; rcz++)
	{
		for (int rcx = 0; rcx <= MAX_CHUNK; rcx++)
		{
			// get the actual chunk from its rotated coordinates
			// use the "new" chunk saved by the previous iteration if possible
			chunk = rcx > 0 && prev_chunk != NULL ? new_chunk :
					get_chunk(reg, rcx, rcz, opts.rotate);
			if (chunk == NULL)
			{
				if (rcx > 0) nbt_free(prev_chunk);
				prev_chunk = NULL;
				continue;
			}

			// get neighbouring chunks, either from this region or a neighbouring one
			nchunks[0] = rcz > 0 ? get_chunk(reg, rcx, rcz - 1, opts.rotate) :
					(!nregions[0].loaded ? NULL :
							get_chunk(nregions[0], rcx, MAX_CHUNK, opts.rotate));

			nchunks[1] = rcx < MAX_CHUNK ? get_chunk(reg, rcx + 1, rcz, opts.rotate) :
					(!nregions[1].loaded ? NULL :
							get_chunk(nregions[1], 0, rcz, opts.rotate));

			nchunks[2] = rcz < MAX_CHUNK ? get_chunk(reg, rcx, rcz + 1, opts.rotate) :
					(!nregions[2].loaded ? NULL :
							get_chunk(nregions[2], rcx, 0, opts.rotate));

			nchunks[3] = rcx > 0 ? prev_chunk :
					(!nregions[3].loaded ? NULL :
							get_chunk(nregions[3], MAX_CHUNK, rcz, opts.rotate));

			// render chunk image onto region image
			int cpx, cpy;
			if (opts.isometric)
			{
				// translate orthographic to isometric coordinates
				cpx = rpx + (rcx + MAX_CHUNK - rcz) * ISO_CHUNK_X_MARGIN;
				cpy = rpy + (rcx + rcz) * ISO_CHUNK_Y_MARGIN;
			}
			else
			{
				cpx = rpx + rcx * CHUNK_BLOCK_LENGTH;
				cpy = rpy + rcz * CHUNK_BLOCK_LENGTH;
			}
			render_chunk_map(image, cpx, cpy, chunk, nchunks, tex, opts);

			// free chunks, or save them for the next iteration if we're not at the end of a row
			nbt_free(nchunks[0]);
			nbt_free(nchunks[2]);
			nbt_free(nchunks[3]);
			if (rcx == MAX_CHUNK)
			{
				nbt_free(chunk);
				nbt_free(nchunks[1]);
			}
			else
			{
				prev_chunk = chunk;
				new_chunk = nchunks[1];
			}
		}
	}

	fclose(reg.file);
}


void save_region_map(const char *regionfile, const char *imagefile, const options opts)
{
	image rimage = opts.isometric ?
			create_image(ISO_REGION_WIDTH, ISO_REGION_HEIGHT) :
			create_image(REGION_BLOCK_LENGTH, REGION_BLOCK_LENGTH);

	textures *tex = read_textures(opts.texpath, opts.shapepath);

	clock_t start = clock();
	render_region_map(&rimage, 0, 0, regionfile, NULL, tex, opts);
	clock_t render_end = clock();
	printf("Total render time: %f seconds\n", (double)(render_end - start) / CLOCKS_PER_SEC);

	free_textures(tex);

	if (rimage.data == NULL) return;

	printf("Saving image to %s ...\n", imagefile);
	save_image(rimage, imagefile);
	printf("Total save time: %f seconds\n", (double)(clock() - render_end) / CLOCKS_PER_SEC);

	free_image(rimage);
}
