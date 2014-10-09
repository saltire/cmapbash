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

#include "nbt.h"

#include "data.h"
#include "dims.h"


#define SECTOR_BYTES 4096
#define OFFSET_BYTES 4
#define LENGTH_BYTES 4


FILE *open_region_file(region *reg)
{
	if (reg == NULL) return NULL;
	reg->file = fopen(reg->path, "rb");
	if (reg->file == NULL) fprintf(stderr, "Error %d reading region file: %s\n", errno, reg->path);
	return reg->file;
}


void close_region_file(region *reg)
{
	if (reg == NULL || reg->file == NULL) return;
	fclose(reg->file);
	reg->file = NULL;
}


region *read_region(const char *regiondir, const int rx, const int rz,
		const unsigned int *rblimits)
{
	region *reg = (region*)malloc(sizeof(region));
	reg->x = rx;
	reg->z = rz;
	sprintf(reg->path, "%s/r.%d.%d.mca", regiondir, rx, rz);

	open_region_file(reg);
	if (reg->file == NULL) return NULL;

	memset(reg->offsets, 0, sizeof(reg->offsets));
	memset(reg->cblimits, 0, sizeof(reg->cblimits));

	// default chunk limits, for looping below
	unsigned int rclimits[4] = {0, MAX_REGION_CHUNK, MAX_REGION_CHUNK, 0};

	// store block/chunk limits for the region
	if (rblimits == NULL) reg->blimits = NULL;
	else {
		reg->blimits = (unsigned int*)malloc(4 * sizeof(int));
		for (int i = 0; i < 4; i++)
		{
			reg->blimits[i] = rblimits[i];
			rclimits[i] = rblimits[i] >> CHUNK_BLOCK_BITS;
		}
	}

	for (unsigned int cz = rclimits[0]; cz <= rclimits[2]; cz++)
	{
		for (unsigned int cx = rclimits[3]; cx <= rclimits[1]; cx++)
		{
			unsigned int co = cz * REGION_CHUNK_LENGTH + cx;

			// store this chunk's byte offset in the region file
			fseek(reg->file, co * OFFSET_BYTES, SEEK_SET);
			unsigned char buffer[OFFSET_BYTES];
			fread(buffer, 1, OFFSET_BYTES, reg->file);
			reg->offsets[co] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];

			// store block limits for this chunk
			if (reg->blimits != NULL)
			{
				// default limits, for chunk edges that aren't on a region edge
				unsigned int cblimits[4] = {0, MAX_CHUNK_BLOCK, MAX_CHUNK_BLOCK, 0};
				int edge = 0;

				// check if this chunk is on any edge of this region
				for (int i = 0; i < 4; i++)
					if ((i % 2 ? cx : cz) == rclimits[i])
					{
						edge = 1;
						// set the chunk's limit to the region's limit for this edge
						cblimits[i] = reg->blimits[i] & MAX_CHUNK_BLOCK;
					}

				// if no edges (all defaults), set chunk limits to null
				if (edge)
				{
					reg->cblimits[co] = (unsigned int*)malloc(4 * sizeof(int));
					memcpy(reg->cblimits[co], cblimits, 4 * sizeof(int));
				}
				else reg->cblimits[co] = NULL;
			}
		}
	}

	close_region_file(reg);
	return reg;
}


void free_region(region *reg)
{
	free(reg->blimits);
	for (int i = 0; i < REGION_CHUNK_AREA; i++) free(reg->cblimits[i]);
	free(reg);
}


static unsigned int get_chunk_offset(const unsigned int rcx, const unsigned int rcz,
		const char rotate)
{
	return get_offset(0, rcx, rcz, REGION_CHUNK_LENGTH, rotate);
}


char chunk_exists(const region *reg, const unsigned int rcx, const unsigned int rcz,
		const unsigned char rotate)
{
	return reg->offsets[get_chunk_offset(rcx, rcz, rotate)] != 0;
}


chunk_data *read_chunk(const region *reg, const unsigned int rcx, const unsigned int rcz,
		const char rotate, const chunk_flags *flags, const unsigned int *ylimits)
{
	// warning: this function assumes that the region's file is already open
	if (reg == NULL || reg->file == NULL) return NULL;

	// get the byte offset of this chunk's data in the region file
	unsigned int co = get_chunk_offset(rcx, rcz, rotate);
	unsigned int offset = reg->offsets[co];
	if (offset == 0) return NULL;

	unsigned char buffer[LENGTH_BYTES];
	fseek(reg->file, offset * SECTOR_BYTES, SEEK_SET);
	fread(buffer, 1, LENGTH_BYTES, reg->file);
	int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

	char *cdata = (char*)malloc(length);
	fseek(reg->file, 1, SEEK_CUR);
	//printf("Reading %d bytes at %#lx.\n", length, ftell(rfile));
	fread(cdata, length, 1, reg->file);

	chunk_data *chunk = (chunk_data*)malloc(sizeof(chunk_data));
	chunk->nbt = nbt_parse_compressed(cdata, length);
	if (errno != NBT_OK)
	{
		fprintf(stderr, "Error %d parsing chunk\n", errno);
		return NULL;
	}
	free(cdata);

	// get chunk's block limits from the region if they exist
	chunk->blimits = reg->cblimits[co];

	// get chunk's byte data
	chunk->bids = flags->bids ? get_chunk_data(chunk, "Blocks", 0, 0, ylimits) : NULL;
	chunk->bdata = flags->bdata ? get_chunk_data(chunk, "Data", 1, 0, ylimits) : NULL;
	chunk->blight = flags->blight ? get_chunk_data(chunk, "BlockLight", 1, 0, ylimits) : NULL;
	chunk->slight = flags->slight ? get_chunk_data(chunk, "SkyLight", 1, 255, ylimits) : NULL;

	return chunk;
}


void free_chunk(chunk_data *chunk)
{
	if (chunk == NULL) return;
	nbt_free(chunk->nbt);
	free(chunk->bids);
	free(chunk->bdata);
	free(chunk->blight);
	free(chunk->slight);
	free(chunk);
}
