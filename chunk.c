#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"


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


void combine_alpha(unsigned char* top, unsigned char* bottom)
{
	if (top[3] == 255) return;

	// combine the two colours, storing the result in the first colour's buffer
	for (int ch = 0; ch < CHANNELS - 1; ch++)
	{
		top[ch] = (top[ch] * top[3] + bottom[ch] * bottom[3] * (255 - top[3]) / 255) / 255;
	}
	top[3] = top[3] + bottom[3] - top[3] * bottom[3] / 255;
}


void get_block_colour_at_height(unsigned char* blocks, int b, int y,
		const unsigned char* colours, unsigned char* pixel)
{
	// copy the block colour into the pixel buffer
	unsigned char blockid = blocks[y * CHUNK_BLOCK_AREA + b];
	memcpy(pixel, &colours[blockid * CHANNELS], CHANNELS);

	// if block colour is not fully opaque, get the next block and combine with this one
	if (pixel[3] < 255)
	{
		unsigned char next[CHANNELS] = {0};
		if (y > 0)
		{
			get_block_colour_at_height(blocks, b, y - 1, colours, (unsigned char*)next);
		}
		combine_alpha(pixel, next);
	}
}


void get_block_colour(unsigned char* blocks, const unsigned char* colours, int b,
		unsigned char* pixel, char alpha)
{
	unsigned char blockid;
	for (int y = CHUNK_BLOCK_HEIGHT - 1; y >= 0; y--)
	{
		blockid = blocks[y * CHUNK_BLOCK_AREA + b];
		if (blockid >= BLOCK_TYPES) blockid = 0; // unknown block type defaults to air
		if (blockid != 0)
		{
			if (alpha)
			{
				get_block_colour_at_height(blocks, b, y, colours, pixel);
			}
			else
			{
				// copy the block colour into the pixel buffer, setting alpha to full
				memcpy(pixel, &colours[blockid * CHANNELS], CHANNELS - 1);
				pixel[3] = 255;
			}
			break;
		}
	}
}


unsigned char* render_chunk_blockmap(nbt_node* chunk, const unsigned char* colours,
		const char alpha)
{
	unsigned char* blocks = get_chunk_blocks(chunk);
	unsigned char* image = (unsigned char*)calloc(CHUNK_BLOCK_AREA * CHANNELS, sizeof(char));

	for (int b = 0; b < CHUNK_BLOCK_AREA; b++)
	{
		get_block_colour(blocks, colours, b, &image[b * CHANNELS], alpha);
	}
	free(blocks);
	return image;
}


unsigned char* render_chunk_heightmap(nbt_node* chunk)
{
	unsigned char* heightmap = get_chunk_heightmap(chunk);
	unsigned char* image = (unsigned char*)calloc(CHUNK_BLOCK_AREA * CHANNELS, sizeof(char));
	for (int b = 0; b < CHUNK_BLOCK_AREA; b++)
	{
		unsigned char* pixel = &image[b * CHANNELS];
		//printf("Rendering height %d at (%d, %d)\n",
		//		(int)heightmap[b], b % CHUNKWIDTH, b / CHUNKWIDTH);

		// colour values must be big-endian, so copy them manually
		for (int c = 0; c < CHANNELS - 1; c++)
		{
			pixel[c] = heightmap[b];
		}
		pixel[3] = 255;
	}
	free(heightmap);
	return image;
}


void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const unsigned char* colours,
		const char alpha)
{
	unsigned char* chunkimage = render_chunk_blockmap(chunk, colours, alpha);
	lodepng_encode32_file(imagefile, chunkimage, CHUNK_BLOCK_WIDTH, CHUNK_BLOCK_WIDTH);
	free(chunkimage);
}


void save_chunk_heightmap(nbt_node* chunk, const char* imagefile)
{
	unsigned char* chunkimage = render_chunk_heightmap(chunk);
	lodepng_encode32_file(imagefile, chunkimage, CHUNK_BLOCK_WIDTH, CHUNK_BLOCK_WIDTH);
	free(chunkimage);
}
