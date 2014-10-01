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


#define SECTOR_LENGTH 4096
#define OFFSET_LENGTH 4


static FILE *open_region_file(region *reg)
{
	if (reg == NULL) return NULL;
	reg->file = fopen(reg->path, "rb");
	if (reg->file == NULL) fprintf(stderr, "Error %d reading region file: %s\n", errno, reg->path);
	return reg->file;
}


static void close_region_file(region *reg)
{
	if (reg == NULL || reg->file == NULL) return;
	fclose(reg->file);
	reg->file = NULL;
}


region read_region(const char *regiondir, const int rx, const int rz)
{
	region reg;
	reg.x = rx;
	reg.z = rz;
	sprintf(reg.path, "%s/r.%d.%d.mca", regiondir, rx, rz);

	open_region_file(&reg);
	if (reg.file == NULL) reg.loaded = 0;
	else
	{
		for (int c = 0; c < REGION_CHUNK_AREA; c++)
		{
			unsigned char buffer[OFFSET_LENGTH];
			fread(buffer, 1, OFFSET_LENGTH, reg.file);
			reg.offsets[c] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
		}
		reg.loaded = 1;
	}
	close_region_file(&reg);
	return reg;
}


void get_region_margins(int *margins, region *reg, const char rotate, const char isometric)
{
	open_region_file(reg);
	if (reg == NULL || reg->file == NULL) return;

	for (int i = 0; i < 4; i++)
	{
		if (isometric) margins[i] = i % 2 ? ISO_REGION_WIDTH : ISO_REGION_TOP_HEIGHT;
		else margins[i] = REGION_BLOCK_LENGTH;
	}

	unsigned char buffer[OFFSET_LENGTH];
	unsigned int offset;
	for (int cz = 0; cz < REGION_CHUNK_LENGTH; cz++)
	{
		for (int cx = 0; cx < REGION_CHUNK_LENGTH; cx++)
		{
			if (reg->offsets[cz * REGION_CHUNK_LENGTH + cx] > 0)
			{
				// chunk offsets for this chunk
				int co[] =
				{
					cz,
					(MAX_REGION_CHUNK - cx),
					(MAX_REGION_CHUNK - cz),
					cx
				};

				// rotated pixel margins for this chunk
				int rcm[4];
				for (int i = 0; i < 4; i++)
				{
					int m = (i + rotate) % 4;
					rcm[m] = isometric ? ((co[i] + co[(i + 3) % 4])
							* (m % 2 ? ISO_CHUNK_X_MARGIN : ISO_CHUNK_Y_MARGIN))
							: co[i] * CHUNK_BLOCK_LENGTH;

					// assign to rotated region margins if lower
					if (rcm[m] < margins[m]) margins[m] = rcm[m];
				}
			}
		}
	}
	close_region_file(reg);
}


static int get_chunk_offset(const region *reg, const int rcx, const int rcz, const char rotate)
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
		cz = MAX_REGION_CHUNK - rcx;
		break;
	case 2:
		cx = MAX_REGION_CHUNK - rcx;
		cz = MAX_REGION_CHUNK - rcz;
		break;
	case 3:
		cx = MAX_REGION_CHUNK - rcz;
		cz = rcx;
		break;
	}

	return reg->offsets[cz * REGION_CHUNK_LENGTH + cx];
}


static nbt_node *read_chunk(const region *reg, const int rcx, const int rcz, const char rotate)
{
	// warning: this function assumes that the region's file is already open
	if (reg == NULL || reg->file == NULL) return NULL;

	int offset = get_chunk_offset(reg, rcx, rcz, rotate);
	if (offset == 0) return NULL;

	unsigned char buffer[OFFSET_LENGTH];
	fseek(reg->file, offset * SECTOR_LENGTH, SEEK_SET);
	fread(buffer, 1, OFFSET_LENGTH, reg->file);
	int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

	char *cdata = (char*)malloc(length);
	fseek(reg->file, 1, SEEK_CUR);
	//printf("Reading %d bytes at %#lx.\n", length, ftell(rfile));
	fread(cdata, length, 1, reg->file);

	nbt_node *chunk_nbt = nbt_parse_compressed(cdata, length);
	if (errno != NBT_OK)
	{
		fprintf(stderr, "Error %d parsing chunk\n", errno);
		return NULL;
	}
	free(cdata);

	return chunk_nbt;
}


void render_tiny_region_map(image *image, const int rpx, const int rpy, region *reg,
		const options *opts)
{
	open_region_file(reg);
	if (reg == NULL || reg->file == NULL) return;

	unsigned char colour[CHANNELS] = {255, 255, 255, 255};

	for (int rcz = 0; rcz < REGION_CHUNK_LENGTH; rcz++)
	{
		for (int rcx = 0; rcx < REGION_CHUNK_LENGTH; rcx++)
		{
			// check for the chunk's existence using its rotated coordinates
			int offset = get_chunk_offset(reg, rcx, rcz, opts->rotate);
			if (offset == 0) continue;

			// if it exists, colour this pixel
			int cpx = rpx + rcx;
			int cpy = rpy + rcz;
			unsigned char *pixel = &image->data[(cpy * image->width + cpx) * CHANNELS];
			memcpy(pixel, colour, CHANNELS);
		}
	}

	close_region_file(reg);
}


void render_region_map(image *image, const int rpx, const int rpy, region *reg,
		region *nregions[4], const textures *tex, const options *opts)
{
	open_region_file(reg);
	if (reg == NULL || reg->file == NULL) return;

	for (int i = 0; i < 4; i++) open_region_file(nregions[i]);

	nbt_node *chunk, *prev_chunk, *new_chunk, *nchunks[4];
	// use rotated chunk coordinates, since we need to draw them top to bottom for isometric
	for (int rcz = 0; rcz < REGION_CHUNK_LENGTH; rcz++)
	{
		for (int rcx = 0; rcx < REGION_CHUNK_LENGTH; rcx++)
		{
			// get the actual chunk from its rotated coordinates
			// use the "new" chunk saved by the previous iteration if possible
			chunk = rcx > 0 && prev_chunk != NULL ? new_chunk :
					read_chunk(reg, rcx, rcz, opts->rotate);
			if (chunk == NULL)
			{
				if (rcx > 0) nbt_free(prev_chunk);
				prev_chunk = NULL;
				continue;
			}

			// get neighbouring chunks, either from this region or a neighbouring one
			nchunks[0] = rcz > 0 ? read_chunk(reg, rcx, rcz - 1, opts->rotate) :
					read_chunk(nregions[0], rcx, MAX_REGION_CHUNK, opts->rotate);

			nchunks[1] = rcx < MAX_REGION_CHUNK ? read_chunk(reg, rcx + 1, rcz, opts->rotate) :
					read_chunk(nregions[1], 0, rcz, opts->rotate);

			nchunks[2] = rcz < MAX_REGION_CHUNK ? read_chunk(reg, rcx, rcz + 1, opts->rotate) :
					read_chunk(nregions[2], rcx, 0, opts->rotate);

			nchunks[3] = rcx > 0 ? prev_chunk :
					read_chunk(nregions[3], MAX_REGION_CHUNK, rcz, opts->rotate);

			// render chunk image onto region image
			int cpx, cpy;
			if (opts->isometric)
			{
				// translate orthographic to isometric coordinates
				cpx = rpx + (rcx + MAX_REGION_CHUNK - rcz) * ISO_CHUNK_X_MARGIN;
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
			if (rcx == MAX_REGION_CHUNK)
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

	close_region_file(reg);
	for (int i = 0; i < 4; i++) close_region_file(nregions[i]);
}


void save_region_map(const char *regiondir, const int rx, const int rz, const char *imagefile,
		const options *opts)
{
	region reg = read_region(regiondir, rx, rz);

	image rimage = opts->isometric ?
			create_image(ISO_REGION_WIDTH, ISO_REGION_HEIGHT) :
			create_image(REGION_BLOCK_LENGTH, REGION_BLOCK_LENGTH);

	textures *tex = read_textures(opts->texpath, opts->shapepath);

	clock_t start = clock();
	render_region_map(&rimage, 0, 0, &reg, NULL, tex, opts);
	clock_t render_end = clock();
	printf("Total render time: %f seconds\n", (double)(render_end - start) / CLOCKS_PER_SEC);

	free_textures(tex);

	if (rimage.data == NULL) return;

	printf("Saving image to %s ...\n", imagefile);
	save_image(rimage, imagefile);
	printf("Total save time: %f seconds\n", (double)(clock() - render_end) / CLOCKS_PER_SEC);

	free_image(rimage);
}
