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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nbt.h"

#include "data.h"


chunk_data *read_chunk(const region *reg, const uint8_t rcx, const uint8_t rcz,
		const uint8_t rotate, const chunk_flags *flags, const uint8_t *ylimits)
{
	// warning: this function assumes that the region's file is already open
	if (reg == NULL || reg->file == NULL) return NULL;

	// get the byte offset of this chunk's data in the region file
	uint16_t co = get_chunk_offset(rcx, rcz, rotate);
	uint32_t offset = reg->offsets[co];
	if (offset == 0) return NULL;

	uint8_t buffer[LENGTH_BYTES];
	fseek(reg->file, offset * SECTOR_BYTES, SEEK_SET);
	fread(buffer, 1, LENGTH_BYTES, reg->file);
	uint32_t length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

	uint8_t *cdata = (uint8_t*)malloc(length);
	fseek(reg->file, 1, SEEK_CUR);
	//printf("Reading %d bytes at %#lx.\n", length, ftell(rfile));
	fread(cdata, length, 1, reg->file);

	chunk_data *chunk = parse_chunk_nbt(cdata, length, flags, reg->cblimits[co], ylimits);
	free(cdata);

	return chunk;
}


bool chunk_exists(const region *reg, const uint8_t rcx, const uint8_t rcz, const uint8_t rotate)
{
	return reg->offsets[get_chunk_offset(rcx, rcz, rotate)] != 0;
}


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


region *read_region(const char *regiondir, const int32_t rx, const int32_t rz,
		const uint16_t *rblimits)
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
	uint16_t rclimits[4] = {0, MAX_REGION_CHUNK, MAX_REGION_CHUNK, 0};

	// store block/chunk limits for the region
	if (rblimits == NULL) reg->blimits = NULL;
	else {
		reg->blimits = (uint16_t*)malloc(4 * sizeof(uint16_t));
		for (uint8_t i = 0; i < 4; i++)
		{
			reg->blimits[i] = rblimits[i];
			rclimits[i] = rblimits[i] >> CHUNK_BLOCK_BITS;
		}
	}

	for (uint8_t cz = rclimits[0]; cz <= rclimits[2]; cz++)
	{
		for (uint8_t cx = rclimits[3]; cx <= rclimits[1]; cx++)
		{
			uint16_t co = cz * REGION_CHUNK_LENGTH + cx;

			// store this chunk's byte offset in the region file
			fseek(reg->file, co * OFFSET_BYTES, SEEK_SET);
			uint8_t buffer[OFFSET_BYTES];
			fread(buffer, 1, OFFSET_BYTES, reg->file);
			reg->offsets[co] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];

			// store block limits for this chunk
			if (reg->blimits != NULL)
			{
				// default limits, for chunk edges that aren't on a region edge
				uint8_t cblimits[4] = {0, MAX_CHUNK_BLOCK, MAX_CHUNK_BLOCK, 0};
				bool edge = 0;

				// check if this chunk is on any edge of this region
				for (uint8_t i = 0; i < 4; i++)
					if ((i % 2 ? cx : cz) == rclimits[i])
					{
						edge = 1;
						// set the chunk's limit to the region's limit for this edge
						cblimits[i] = reg->blimits[i] & MAX_CHUNK_BLOCK;
					}

				// store the chunk limits if this is an edge chunk, otherwise set to null
				if (edge)
				{
					reg->cblimits[co] = (uint8_t*)malloc(4);
					memcpy(reg->cblimits[co], cblimits, 4);
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
	for (uint32_t i = 0; i < REGION_CHUNK_AREA; i++) free(reg->cblimits[i]);
	free(reg);
}
