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


#include <stdlib.h>
#include <string.h>

#include "nbt.h"

#include "dims.h"


static void copy_section_bytes(unsigned char *data, nbt_node *section, const char *name,
		const unsigned int yo, const unsigned int syolimits[2], const unsigned int *cblimits)
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
		for (unsigned int syo = syolimits[0]; syo < syolimits[1]; syo += CHUNK_BLOCK_AREA)
			for (unsigned int z = cblimits[0]; z <= cblimits[2]; z++)
				for (unsigned int x = cblimits[3]; x <= cblimits[1]; x++)
				{
					unsigned int b = syo + z * CHUNK_BLOCK_LENGTH + x;
					data[yo + b] = array->payload.tag_byte_array.data[b];
				}
}


static void copy_section_nybbles(unsigned char *data, nbt_node *section, const char *name,
		const unsigned int yo, const unsigned int syolimits[2], const unsigned int *cblimits)
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
		for (unsigned int b = syolimits[0]; b < syolimits[1]; b += 2)
		{
			unsigned char byte = array->payload.tag_byte_array.data[b / 2];
			data[yo + b] = byte & 0xf;
			data[yo + b + 1] = byte >> 4;
		}
	else
		for (unsigned int syo = syolimits[0]; syo < syolimits[1]; syo += CHUNK_BLOCK_AREA)
			for (unsigned int z = cblimits[0]; z <= cblimits[2]; z++)
				for (unsigned int x = cblimits[3]; x <= cblimits[1]; x += 2)
				{
					unsigned int b = syo + z * CHUNK_BLOCK_LENGTH + x;
					unsigned char byte = array->payload.tag_byte_array.data[b / 2];
					data[yo + b] = byte & 0xf;
					data[yo + b + 1] = byte >> 4;
				}
}


unsigned char *get_chunk_data(nbt_node *chunk_nbt, unsigned char *tag, char half,
		char defval, const unsigned int *cblimits, const unsigned int *ylimits)
{
	unsigned char *data = (unsigned char*)malloc(CHUNK_BLOCK_VOLUME);
	memset(data, defval, CHUNK_BLOCK_VOLUME);

	nbt_node *sections = nbt_find_by_name(chunk_nbt, "Sections");
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
				unsigned int yo = sy * SECTION_BLOCK_VOLUME;
				unsigned int syolimits[2] = {0, SECTION_BLOCK_VOLUME};
				if (ylimits != NULL)
				{
					if (ylimits[0] / SECTION_BLOCK_HEIGHT == sy)
						syolimits[0] = (ylimits[0] % SECTION_BLOCK_HEIGHT) * CHUNK_BLOCK_AREA;
					if (ylimits[1] / SECTION_BLOCK_HEIGHT == sy)
						syolimits[1] = (ylimits[1] % SECTION_BLOCK_HEIGHT + 1) * CHUNK_BLOCK_AREA;
				}

				if (half)
					copy_section_nybbles(data, section->data, tag, yo, syolimits, cblimits);
				else
					copy_section_bytes(data, section->data, tag, yo, syolimits, cblimits);
			}
		}
	}

	return data;
}
