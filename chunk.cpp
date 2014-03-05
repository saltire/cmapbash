#include "nbt.h"

#include <iostream>
#include <stdio.h>

using namespace std;

#define BLOCKS 16
#define SECHEIGHT 16
#define SECTIONS 16
#define CHUNKSIZE BLOCKS * BLOCKS
#define SECSIZE SECHEIGHT * CHUNKSIZE


struct nbt_list {
	struct nbt_node* data; /* A single node's data. */
	struct list_head entry;
};


unsigned char* get_chunk_heightmap(nbt_node* chunk)
{
	unsigned char* heightmap = (unsigned char*)malloc(CHUNKSIZE);

	nbt_node* hdata = nbt_find_by_name(chunk, "HeightMap");
	if (hdata->type == TAG_INT_ARRAY)
	{
		for (int i = 0; i < CHUNKSIZE; i++)
		{
			heightmap[i] = *(hdata->payload.tag_int_array.data + i) & 0xff;
		}
	}

	return heightmap;
}


unsigned char* get_chunk_blocks(nbt_node* chunk)
{
	unsigned char* blocks = (unsigned char*)calloc(SECTIONS * SECSIZE, sizeof(char));

	nbt_node* sections = nbt_find_by_name(chunk, "Sections");
	if (sections->type == TAG_LIST)
	{
		struct list_head* pos;
		list_for_each(pos, &sections->payload.tag_list->entry)
		{
			struct nbt_list* section = list_entry(pos, struct nbt_list, entry);
			if (section->data->type == TAG_COMPOUND)
			{
				nbt_node* ynode = nbt_find_by_name(section->data, "Y");
				if (ynode->type != TAG_BYTE)
				{
					cout << "Problem parsing sections." << endl;
				}
				int8_t y = ynode->payload.tag_byte;

				nbt_node* bdata = nbt_find_by_name(section->data, "Blocks");
				if (bdata->type != TAG_BYTE_ARRAY ||
						bdata->payload.tag_byte_array.length != SECSIZE)
				{
					cout << "Problem parsing blocks." << endl;
				}

				cout << "Copying " << SECSIZE << " blocks from section " << y << endl;
				memcpy(blocks + y * SECSIZE, bdata->payload.tag_byte_array.data, SECSIZE);
			}
		}
	}

	return blocks;
}
