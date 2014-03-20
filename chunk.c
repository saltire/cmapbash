#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "image.h"
#include "textures.h"


static void copy_section_byte_data(nbt_node* section, char* name, unsigned char* data, int8_t y)
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


static void copy_section_half_byte_data(nbt_node* section, char* name, unsigned char* data,
		int8_t y)
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
				if (blight != NULL)
				{
					copy_section_half_byte_data(section->data, "BlockLight", blight, y);
				}
			}
		}
	}
}


static void get_block_colour_at_height(unsigned char* pixel, unsigned char* blocks,
		unsigned char* data, const texture* textures, int b, int y)
{
	// copy the block colour into the pixel buffer
	unsigned char blockid = blocks[y * CHUNK_BLOCK_AREA + b];
	unsigned char type = data[y * CHUNK_BLOCK_AREA + b] % textures[blockid].mask;
	memcpy(pixel, &textures[blockid].types[type].colour, CHANNELS);

	// if block colour is not fully opaque, get the next block and combine with this one
	if (pixel[ALPHA] < 255)
	{
		unsigned char next[CHANNELS] = {0};
		if (y > 0)
		{
			get_block_colour_at_height(next, blocks, data, textures, b, y - 1);
		}
		combine_alpha(pixel, next, 0);
	}
}


static void get_block_colour(unsigned char* pixel, unsigned char* blocks, unsigned char* data,
		unsigned char* blight, const texture* textures, int b)
{
	for (int y = CHUNK_BLOCK_HEIGHT - 1; y >= 0; y--)
	{
		unsigned char blockid = blocks[y * CHUNK_BLOCK_AREA + b];
		if (blockid >= BLOCK_TYPES) blockid = 0; // unknown block type defaults to air
		if (blockid != 0)
		{
			get_block_colour_at_height(pixel, blocks, data, textures, b, y);
			adjust_colour_by_height(pixel, y);
			if (blight != NULL)
			{
				adjust_colour_by_lum(pixel, blight[y * CHUNK_BLOCK_AREA + b]);
			}
			break;
		}
	}
}


int get_rotated_index(const int x, const int z, const int length, const unsigned char rotate)
{
	int bx, bz;
	switch(rotate) {
	case 0:
		bx = x;
		bz = z;
		break;
	case 1:
		bx = z;
		bz = length - x - 1;
		break;
	case 2:
		bx = length - x - 1;
		bz = length - z - 1;
		break;
	case 3:
		bx = length - z - 1;
		bz = x;
		break;
	}
	return (bz * length + bx);
}


image render_chunk_blockmap(nbt_node* chunk, const texture* textures, const char night,
		const char rotate)
{
	image cimage;
	cimage.width = CHUNK_BLOCK_LENGTH;
	cimage.height = CHUNK_BLOCK_LENGTH;
	cimage.data = (unsigned char*)calloc(CHUNK_BLOCK_AREA * CHANNELS, sizeof(char));

	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* data = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* blight = NULL;
	if (night)
	{
		blight = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	}
	get_chunk_blockdata(chunk, blocks, data, blight);

	for (int z = 0; z < CHUNK_BLOCK_LENGTH; z++)
	{
		for (int x = 0; x < CHUNK_BLOCK_LENGTH; x++)
		{
			int b = get_rotated_index(x, z, CHUNK_BLOCK_LENGTH, rotate);
			get_block_colour(&cimage.data[(z * CHUNK_BLOCK_LENGTH + x) * CHANNELS],
					blocks, data, blight, textures, b);
		}
	}
	free(blocks);
	free(data);
	free(blight);
	return cimage;
}


image render_chunk_iso_blockmap(nbt_node* chunk, const texture* textures, const char night,
		const char rotate)
{
	image cimage;
	cimage.width = ISO_CHUNK_WIDTH;
	cimage.height = ISO_CHUNK_HEIGHT;
	cimage.data = (unsigned char*)calloc(cimage.width * cimage.height * CHANNELS, sizeof(char));
	//printf("Rendering isometric image, %d x %d\n", cimage.width, cimage.height);

	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* data = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* blight = NULL;
	if (night)
	{
		blight = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	}
	get_chunk_blockdata(chunk, blocks, data, blight);

	for (int z = 0; z < CHUNK_BLOCK_LENGTH; z++)
	{
		for (int x = CHUNK_BLOCK_LENGTH - 1; x >= 0; x--)
		{
			int bh = get_rotated_index(x, z, CHUNK_BLOCK_LENGTH, rotate);

			for (int by = 0; by < CHUNK_BLOCK_HEIGHT; by++)
			{
				int b = by * CHUNK_BLOCK_AREA + bh;
				unsigned char blockid = blocks[b];
				if (blockid == 0 || blockid >= BLOCK_TYPES) continue;

				unsigned char type = data[b] % textures[blockid].mask;

				unsigned char colour[CHANNELS];
				memcpy(&colour, &textures[blockid].types[type].colour, CHANNELS);
				adjust_colour_by_height(colour, by);
				if (blight != NULL)
				{
					adjust_colour_by_lum(colour, blight[b]);
				}

				int px = (x + z) * ISO_BLOCK_WIDTH / 2;
				int py = (CHUNK_BLOCK_LENGTH - x + z - 1) * ISO_BLOCK_TOP_HEIGHT
						+ (CHUNK_BLOCK_HEIGHT - by - 1) * ISO_BLOCK_DEPTH;
				//printf("Block %d,%d,%d rendering at pixel %d,%d\n", bx, bz, by, px, py);

				for (int sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
				{
					// before drawing the top of this block, check the block above
					if (sy < ISO_BLOCK_TOP_HEIGHT && by < CHUNK_BLOCK_HEIGHT - 1)
					{
						// if the block above is the same type, don't draw the top of this one
						unsigned char tblockid = blocks[b + CHUNK_BLOCK_AREA];
						if (tblockid == blockid) continue;

						// if the block above is opaque, don't draw the top of this one
						unsigned char ttype = data[b + CHUNK_BLOCK_AREA] % textures[tblockid].mask;
						if (textures[tblockid].types[ttype].colour[ALPHA] == 255) continue;
					}

					for (int sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
					{
						if (textures[blockid].types[type].shape[sy * ISO_BLOCK_WIDTH + sx])
						{
							combine_alpha(colour,
									&cimage.data[((py + sy) * cimage.width + px + sx) * CHANNELS],
									1);
						}
					}
				}
			}
		}
	}
	free(blocks);
	free(data);
	free(blight);
	return cimage;
}


void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate)
{
	image cimage = isometric
			? render_chunk_iso_blockmap(chunk, textures, night, rotate)
			: render_chunk_blockmap(chunk, textures, night, rotate);

	printf("Saving image to %s ...\n", imagefile);
	SAVE_IMAGE(cimage, imagefile);
	free(cimage.data);
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


image render_chunk_heightmap(nbt_node* chunk)
{
	image cimage;
	cimage.width = CHUNK_BLOCK_LENGTH;
	cimage.height = CHUNK_BLOCK_LENGTH;
	cimage.data = (unsigned char*)calloc(CHUNK_BLOCK_AREA * CHANNELS, sizeof(char));

	unsigned char* heightmap = get_chunk_heightmap(chunk);

	for (int b = 0; b < CHUNK_BLOCK_AREA; b++)
	{
		unsigned char* pixel = &cimage.data[b * CHANNELS];
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
	return cimage;
}


void save_chunk_heightmap(nbt_node* chunk, const char* imagefile)
{
	image cimage = render_chunk_heightmap(chunk);

	printf("Saving image to %s ...\n", imagefile);
	SAVE_IMAGE(cimage, imagefile);
	free(cimage.data);
}
