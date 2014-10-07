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

#include "dims.h"
#include "regiondata.h"


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


region read_region(const char *regiondir, const int rx, const int rz,
		const unsigned int rblimits[4])
{
	region reg;
	reg.x = rx;
	reg.z = rz;
	sprintf(reg.path, "%s/r.%d.%d.mca", regiondir, rx, rz);

	open_region_file(&reg);
	if (reg.file == NULL) reg.loaded = 0;
	else
	{
		memcpy(&reg.blimits, rblimits, sizeof(reg.blimits));
		for (int i = 0; i < 4; i++) reg.climits[i] = rblimits[i] >> CHUNK_BLOCK_BITS;

		memset(reg.offsets, 0, sizeof(reg.offsets));
		for (unsigned int cz = reg.climits[0]; cz <= reg.climits[2]; cz++)
		{
			for (unsigned int cx = reg.climits[3]; cx <= reg.climits[1]; cx++)
			{
				unsigned int co = cz * REGION_CHUNK_LENGTH + cx;
				fseek(reg.file, co * OFFSET_BYTES, SEEK_SET);
				unsigned char buffer[OFFSET_BYTES];
				fread(buffer, 1, OFFSET_BYTES, reg.file);
				reg.offsets[co] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
			}
		}
		reg.loaded = 1;
	}
	close_region_file(&reg);
	return reg;
}


unsigned int get_chunk_offset(const unsigned int rcx, const unsigned int rcz,
		const char rotate)
{
	// get absolute chunk coordinates from rotated chunk coordinates
	unsigned int cx, cz;
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

	return cz * REGION_CHUNK_LENGTH + cx;
}


char chunk_exists(const region *reg, const unsigned int rcx, const unsigned int rcz,
		const unsigned char rotate)
{
	return reg->offsets[get_chunk_offset(rcx, rcz, rotate)] != 0;
}


nbt_node *read_chunk(const region *reg, const int rcx, const int rcz, const char rotate)
{
	// warning: this function assumes that the region's file is already open
	if (reg == NULL || reg->file == NULL) return NULL;

	// get the byte offset of this chunk's data in the region file
	unsigned int offset = reg->offsets[get_chunk_offset(rcx, rcz, rotate)];
	if (offset == 0) return NULL;

	unsigned char buffer[LENGTH_BYTES];
	fseek(reg->file, offset * SECTOR_BYTES, SEEK_SET);
	fread(buffer, 1, LENGTH_BYTES, reg->file);
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
