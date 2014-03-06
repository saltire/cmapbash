#include <stdio.h>
#include <stdlib.h>

#include "nbt.h"

#include "lodepng.h"

#include "chunk.h"
#include "colours.h"


unsigned char* get_chunk_blocks(nbt_node* chunk)
{
	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));

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
					printf("Problem parsing sections.\n");
				}
				int8_t y = ynode->payload.tag_byte;

				nbt_node* bdata = nbt_find_by_name(section->data, "Blocks");
				if (bdata->type != TAG_BYTE_ARRAY ||
						bdata->payload.tag_byte_array.length != SECTION_BLOCK_VOLUME)
				{
					printf("Problem parsing blocks.\n");
				}

				//printf("Copying %d blocks from section %d\n", SECSIZE, y);
				memcpy(blocks + y * SECTION_BLOCK_VOLUME, bdata->payload.tag_byte_array.data,
						SECTION_BLOCK_VOLUME);
			}
		}
	}

	return blocks;
}


unsigned char* get_chunk_heightmap(nbt_node* chunk)
{
	unsigned char* heightmap = (unsigned char*)malloc(CHUNK_BLOCK_AREA);

	nbt_node* hdata = nbt_find_by_name(chunk, "HeightMap");
	if (hdata->type == TAG_INT_ARRAY)
	{
		for (int i = 0; i < CHUNK_BLOCK_AREA; i++)
		{
			heightmap[i] = *(hdata->payload.tag_int_array.data + i) & 0xff;
		}
	}

	return heightmap;
}


unsigned char* render_chunk_blockmap(nbt_node* chunk, const char* colourfile)
{
	unsigned char* blocks = get_chunk_blocks(chunk);
	unsigned char* colours = read_colours(colourfile);
	unsigned char* image = (unsigned char*)calloc(CHUNK_BLOCK_AREA * 4, sizeof(char));

	int offset;
	unsigned char blockid;
	for (int b = 0; b < CHUNK_BLOCK_AREA; b++)
	{
		for (int y = CHUNK_BLOCK_HEIGHT - 1; y >= 0; y--)
		{
			blockid = blocks[y * CHUNK_BLOCK_AREA + b];
			if (blockid != 0)
			{
				//printf("Writing blockid %d at (%d, %d, %d)\n", (int)blockid,
				//		b % CHUNKWIDTH, b / CHUNKWIDTH, y);

				// colour values must be big-endian, so copy them manually
				for (char c = 0; c < 4; c++)
				{
					image[b * 4 + c] = colours[blockid * 4 + c];
				}
				break;
			}
		}
	}
	free(blocks);
	free(colours);
	return image;
}


unsigned char* render_chunk_heightmap(nbt_node* chunk)
{
	unsigned char* heightmap = get_chunk_heightmap(chunk);
	unsigned char* image = (unsigned char*)calloc(CHUNK_BLOCK_AREA * 4, sizeof(char));
	for (int b = 0; b < CHUNK_BLOCK_AREA; b++)
	{
		//printf("Rendering height %d at (%d, %d)\n",
		//		(int)heightmap[b], b % CHUNKWIDTH, b / CHUNKWIDTH);

		// colour values must be big-endian, so copy them manually
		image[b * 4] = heightmap[b];
		image[b * 4 + 1] = heightmap[b];
		image[b * 4 + 2] = heightmap[b];
		image[b * 4 + 3] = 255;
	}
	free(heightmap);
	return image;
}


void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const char* colourfile)
{
	unsigned char* chunkimage = render_chunk_blockmap(chunk, colourfile);
	lodepng_encode32_file(imagefile, chunkimage, CHUNK_BLOCK_WIDTH, CHUNK_BLOCK_WIDTH);
	free(chunkimage);
}


void save_chunk_heightmap(nbt_node* chunk, const char* imagefile)
{
	unsigned char* chunkimage = render_chunk_heightmap(chunk);
	lodepng_encode32_file(imagefile, chunkimage, CHUNK_BLOCK_WIDTH, CHUNK_BLOCK_WIDTH);
	free(chunkimage);
}
