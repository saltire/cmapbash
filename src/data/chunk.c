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
#include <stdlib.h>
#include <string.h>

#include "nbt.h"

#include "data.h"


// get the offset of an item in a rotated x/z array of dimensions length x length
static uint16_t get_offset(const uint8_t rx, const uint8_t rz, const uint8_t y,
		const uint8_t length, const uint8_t rotate)
{
	uint8_t x, z;
	uint8_t max = length - 1;
	switch(rotate) {
	case 0:
		x = rx;
		z = rz;
		break;
	case 1:
		x = rz;
		z = max - rx;
		break;
	case 2:
		x = max - rx;
		z = max - rz;
		break;
	case 3:
		x = max - rz;
		z = rx;
		break;
	}
	return (y * length + z) * length + x;
}


uint16_t get_block_offset(const uint8_t rbx, const uint8_t rbz, const uint8_t y,
		const uint8_t rotate)
{
	return get_offset(rbx, rbz, y, CHUNK_BLOCK_LENGTH, rotate);
}


uint16_t get_chunk_offset(const uint8_t rcx, const uint8_t rcz, const uint8_t rotate)
{
	return get_offset(rcx, rcz, 0, REGION_CHUNK_LENGTH, rotate);
}


void get_neighbour_values(uint8_t nvalues[4], uint8_t *cdata, uint8_t *ncdata[4], uint8_t defval,
		const uint8_t rbx, const uint8_t rbz, const uint8_t y, const uint8_t rotate)
{
	nvalues[TOP] = rbz > 0 ? cdata[get_block_offset(rbx, rbz - 1, y, rotate)] :
			(ncdata[TOP] == NULL ? defval :
					ncdata[TOP][get_block_offset(rbx, MAX_CHUNK_BLOCK, y, rotate)]);

	nvalues[RIGHT] = rbx < MAX_CHUNK_BLOCK ? cdata[get_block_offset(rbx + 1, rbz, y, rotate)] :
			(ncdata[RIGHT] == NULL ? defval :
					ncdata[RIGHT][get_block_offset(0, rbz, y, rotate)]);

	nvalues[BOTTOM] = rbz < MAX_CHUNK_BLOCK ? cdata[get_block_offset(rbx, rbz + 1, y, rotate)] :
			(ncdata[BOTTOM] == NULL ? defval :
					ncdata[BOTTOM][get_block_offset(rbx, 0, y, rotate)]);

	nvalues[LEFT] = rbx > 0 ? cdata[get_block_offset(rbx - 1, rbz, y, rotate)] :
			(ncdata[LEFT] == NULL ? defval :
					ncdata[LEFT][get_block_offset(MAX_CHUNK_BLOCK, rbz, y, rotate)]);
}


// read block data from an NBT byte array at 8 bits per block
static void copy_section_bytes(uint8_t *data, nbt_node *section, const char *name,
		const uint16_t yo, const uint16_t syolimits[2], const uint8_t *cblimits)
{
	nbt_node *array = nbt_find_by_name(section, name);
	if (array->type != TAG_BYTE_ARRAY ||
			array->payload.tag_byte_array.length != SECTION_BLOCK_VOLUME)
	{
		fprintf(stderr, "Problem parsing section byte data.\n");
		return;
	}

	if (cblimits == NULL)
		memcpy(data + yo + syolimits[0], array->payload.tag_byte_array.data + syolimits[0],
				syolimits[1] - syolimits[0]);
	else
		for (uint16_t syo = syolimits[0]; syo < syolimits[1]; syo += CHUNK_BLOCK_AREA)
			for (uint8_t z = cblimits[NORTH]; z <= cblimits[SOUTH]; z++)
				for (uint8_t x = cblimits[WEST]; x <= cblimits[EAST]; x++)
				{
					uint16_t sbo = syo + z * CHUNK_BLOCK_LENGTH + x;
					data[yo + sbo] = array->payload.tag_byte_array.data[sbo];
				}
}


// read block data from an NBT byte array at 4 bits per block
static void copy_section_nybbles(uint8_t *data, nbt_node *section, const char *name,
		const uint16_t yo, const uint16_t syolimits[2], const uint8_t *cblimits)
{
	nbt_node *array = nbt_find_by_name(section, name);
	if (array->type != TAG_BYTE_ARRAY ||
			array->payload.tag_byte_array.length != SECTION_BLOCK_VOLUME / 2)
	{
		fprintf(stderr, "Problem parsing section byte data.\n");
		return;
	}

	// note: syolimits uses < instead of <=
	if (cblimits == NULL)
		for (uint16_t b = syolimits[0]; b < syolimits[1]; b += 2)
		{
			uint8_t byte = array->payload.tag_byte_array.data[b / 2];
			data[yo + b] = byte & 0xf;
			data[yo + b + 1] = byte >> 4;
		}
	else
		for (uint16_t syo = syolimits[0]; syo < syolimits[1]; syo += CHUNK_BLOCK_AREA)
			for (uint8_t z = cblimits[NORTH]; z <= cblimits[SOUTH]; z++)
				// increment x by 2, and read two block values from each byte of data
				for (uint8_t x = cblimits[WEST]; x <= cblimits[EAST]; x += 2)
				{
					uint16_t sbo = syo + z * CHUNK_BLOCK_LENGTH + x;
					uint8_t byte = array->payload.tag_byte_array.data[sbo / 2];
					data[yo + sbo] = byte & 0xf;
					data[yo + sbo + 1] = byte >> 4;
				}
}


// parse a chunk's NBT node for a particular array of byte data
static uint8_t *get_chunk_data(nbt_node *nbt, char *name, const bool half, const uint8_t defval,
		const uint8_t *ylimits, const uint8_t *blimits)
{
	uint8_t *data = (uint8_t*)malloc(CHUNK_BLOCK_VOLUME);
	memset(data, defval, CHUNK_BLOCK_VOLUME);

	nbt_node *sections = nbt_find_by_name(nbt, "Sections");
	if (sections->type == TAG_LIST)
	{
		struct list_head *pos;
		list_for_each(pos, &sections->payload.tag_list->entry)
		{
			struct nbt_list *section = list_entry(pos, struct nbt_list, entry);
			if (section->data->type == TAG_COMPOUND)
			{
				nbt_node *ynode = nbt_find_by_name(section->data, "Y");
				if (ynode->type != TAG_BYTE)
				{
					fprintf(stderr, "Problem parsing sections.\n");
					return NULL;
				}
				int8_t sy = ynode->payload.tag_byte;

				if (ylimits != NULL && (sy < ylimits[0] / SECTION_BLOCK_HEIGHT ||
						sy > ylimits[1] / SECTION_BLOCK_HEIGHT)) continue;

				// get overall section offset, and start/end y offsets for this section
				uint16_t yo = sy * SECTION_BLOCK_VOLUME;
				uint16_t syolimits[2] = {0, SECTION_BLOCK_VOLUME};
				if (ylimits != NULL)
				{
					if (ylimits[0] / SECTION_BLOCK_HEIGHT == sy)
						syolimits[0] = (ylimits[0] % SECTION_BLOCK_HEIGHT) * CHUNK_BLOCK_AREA;
					if (ylimits[1] / SECTION_BLOCK_HEIGHT == sy)
						syolimits[1] = (ylimits[1] % SECTION_BLOCK_HEIGHT + 1) * CHUNK_BLOCK_AREA;
				}

				if (half)
					copy_section_nybbles(data, section->data, name, yo, syolimits, blimits);
				else
					copy_section_bytes(data, section->data, name, yo, syolimits, blimits);
			}
		}
	}

	return data;
}


chunk_data *parse_chunk_nbt(const uint8_t *cdata, const uint32_t length, const chunk_flags *flags,
		uint8_t *cblimits, const uint8_t *ylimits)
{
	chunk_data *chunk = (chunk_data*)malloc(sizeof(chunk_data));
	nbt_node *nbt = nbt_parse_compressed(cdata, length);
	if (errno != NBT_OK)
	{
		fprintf(stderr, "Error %d parsing chunk\n", errno);
		return NULL;
	}

	// get chunk's block limits from the region if they exist
	chunk->blimits = cblimits;

	// get chunk's byte data
	chunk->bids = flags->bids ?
			get_chunk_data(nbt, "Blocks", 0, 0, ylimits, chunk->blimits) : NULL;
	chunk->bdata = flags->bdata ?
			get_chunk_data(nbt, "Data", 1, 0, ylimits, chunk->blimits) : NULL;
	chunk->blight = flags->blight ?
			get_chunk_data(nbt, "BlockLight", 1, 0, ylimits, chunk->blimits) : NULL;
	chunk->slight = flags->slight ?
			get_chunk_data(nbt, "SkyLight", 1, 255, ylimits, chunk->blimits) : NULL;

	if (flags->biomes)
	{
		chunk->biomes = (uint8_t*)malloc(CHUNK_BLOCK_AREA);
		memcpy(chunk->biomes, nbt_find_by_name(nbt, "Biomes")->payload.tag_byte_array.data,
				CHUNK_BLOCK_AREA);
	}
	else chunk->biomes = NULL;

	nbt_free(nbt);

	return chunk;
}


void free_chunk(chunk_data *chunk)
{
	if (chunk == NULL) return;
	free(chunk->bids);
	free(chunk->bdata);
	free(chunk->blight);
	free(chunk->slight);
	free(chunk->biomes);
	free(chunk);
}
