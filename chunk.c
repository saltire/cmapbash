#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"


void copy_section_byte_data(nbt_node* section, char* name, unsigned char* data, int8_t y)
{
	nbt_node* array = nbt_find_by_name(section, name);
	if (array->type != TAG_BYTE_ARRAY ||
			array->payload.tag_byte_array.length != SECTION_BLOCK_VOLUME)
	{
		printf("Problem parsing section byte data.\n");
	}
	memcpy(data + y * SECTION_BLOCK_VOLUME, array->payload.tag_byte_array.data,
			SECTION_BLOCK_VOLUME);
}


void copy_section_half_byte_data(nbt_node* section, char* name, unsigned char* data, int8_t y)
{
	nbt_node* array = nbt_find_by_name(section, name);
	if (array->type != TAG_BYTE_ARRAY ||
			array->payload.tag_byte_array.length != SECTION_BLOCK_VOLUME / 2)
	{
		printf("Problem parsing section byte data.\n");
	}
	for (int i = 0; i < SECTION_BLOCK_VOLUME; i += 2)
	{
		unsigned char byte = array->payload.tag_byte_array.data[i / 2];
		data[y * SECTION_BLOCK_VOLUME + i] = byte % 16;
		data[y * SECTION_BLOCK_VOLUME + i + 1] = byte / 16;
	}
}


void get_chunk_blockdata(nbt_node* chunk, unsigned char* blocks, unsigned char* data,
		unsigned char* blight)
{
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

				copy_section_byte_data(section->data, "Blocks", blocks, y);
				copy_section_half_byte_data(section->data, "Data", data, y);
				copy_section_half_byte_data(section->data, "BlockLight", blight, y);
			}
		}
	}
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
	if (top[ALPHA] == 255) return;

	float bmod = (float)(255 - top[ALPHA]) / 255;
	unsigned char alpha = top[ALPHA] + bottom[ALPHA] * bmod;

	// combine the two colours, storing the result in the first colour's buffer
	for (int ch = 0; ch < ALPHA; ch++)
	{
		top[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
	}
	top[ALPHA] = alpha;
}


void adjust_colour_by_height(unsigned char* pixel, int y)
{
	for (int c = 0; c < ALPHA; c++)
	{
		float contrast = 0.8;
		pixel[c] = (unsigned char)(pixel[c] * contrast + y * (1 - contrast));

		// TODO: a higher contrast height shader
		//int mod = y - 128; // +/- distance from center
		//int limit = 128 - abs(mod); // distance to top or bottom
		//int adjust = (mod / 128) * limit; // amount to adjust
		//pixel[c] = (unsigned char)(pixel[c] + adjust * (1 - contrast));
	}
}


void adjust_colour_by_lum(unsigned char* pixel, unsigned char light)
{
	for (int c = 0; c < ALPHA; c++)
	{
		float darkness = 0.2;
		float lightness = (float)light / 16;
		//printf("Rendering pixel with lightness %d (%f): %d -> ", light, lightness, pixel[c]);
		pixel[c] = (unsigned char)(pixel[c] * (darkness + (1 - darkness) * lightness));
		//printf("%d\n", pixel[c]);
	}
}


void get_block_colour_at_height(unsigned char* pixel, unsigned char* blocks, unsigned char* data,
		unsigned char* blight, const colour* colours, int b, int y)
{
	// copy the block colour into the pixel buffer
	unsigned char blockid = blocks[y * CHUNK_BLOCK_AREA + b];
	unsigned char type = data[y * CHUNK_BLOCK_AREA + b] % colours[blockid].mask;
	memcpy(pixel, &colours[blockid].types[type * CHANNELS], CHANNELS);

	// if block colour is not fully opaque, get the next block and combine with this one
	if (pixel[ALPHA] < 255)
	{
		unsigned char next[CHANNELS] = {0};
		if (y > 0)
		{
			get_block_colour_at_height(next, blocks, data, blight, colours, b, y - 1);
		}
		combine_alpha(pixel, next);
	}
}


void get_block_colour(unsigned char* pixel, unsigned char* blocks, unsigned char* data,
		unsigned char* blight, const colour* colours, int b, char alpha)
{
	for (int y = CHUNK_BLOCK_HEIGHT - 1; y >= 0; y--)
	{
		unsigned char blockid = blocks[y * CHUNK_BLOCK_AREA + b];
		if (blockid >= BLOCK_TYPES) blockid = 0; // unknown block type defaults to air
		if (blockid != 0)
		{
			if (alpha)
			{
				//adjust_colour_by_lum(pixel, blight[y * CHUNK_BLOCK_AREA + b]);

				get_block_colour_at_height(pixel, blocks, data, blight, colours, b, y);

				adjust_colour_by_height(pixel, y);
			}
			else
			{
				unsigned char type = data[y * CHUNK_BLOCK_AREA + b] % colours[blockid].mask;
				// copy the block colour into the pixel buffer, setting alpha to full
				memcpy(pixel, &colours[blockid].types[type * CHANNELS], ALPHA);
				pixel[ALPHA] = 255;
			}
			break;
		}
	}
}


unsigned char* render_chunk_blockmap(nbt_node* chunk, const colour* colours,
		const char alpha)
{
	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* data = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* blight = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));

	get_chunk_blockdata(chunk, blocks, data, blight);
	unsigned char* image = (unsigned char*)calloc(CHUNK_BLOCK_AREA * CHANNELS, sizeof(char));

	for (int b = 0; b < CHUNK_BLOCK_AREA; b++)
	{
		get_block_colour(&image[b * CHANNELS], blocks, data, blight, colours, b, alpha);
	}
	free(blocks);
	free(data);
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
		for (int c = 0; c < ALPHA; c++)
		{
			pixel[c] = heightmap[b];
		}
		pixel[ALPHA] = 255;
	}
	free(heightmap);
	return image;
}


void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const colour* colours,
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
