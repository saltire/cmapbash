#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "image.h"
#include "textures.h"


#define MAX_LIGHT 15
#define MAX_BLOCK (CHUNK_BLOCK_LENGTH - 1)
#define MAX_HEIGHT (CHUNK_BLOCK_HEIGHT - 1)
#define BLOCK_OFFSET(x, z, y) (y * CHUNK_BLOCK_AREA + z * CHUNK_BLOCK_LENGTH + x)

// configurable render options
#define HEIGHT_MIDPOINT 0.3 // midpoint of height shading gradient
#define HEIGHT_CONTRAST 0.7 // amount of contrast for height shading
#define NIGHT_AMBIENCE 0.2 // base light level for night renders
#define EDGE_SHADING 0.125 // amoutn of contrast for topographical highlights and shadows


static void copy_section_bytes(nbt_node* section, char* name, unsigned char* data, int8_t y)
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


static void copy_section_half_bytes(nbt_node* section, char* name, unsigned char* data,
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

				copy_section_bytes(section->data, "Blocks", blocks, y);
				if (data != NULL) copy_section_half_bytes(section->data, "Data", data, y);
				if (blight != NULL) copy_section_half_bytes(section->data, "BlockLight", blight, y);
			}
		}
	}
}


static void get_block_alpha_colour(unsigned char* pixel, unsigned char* blocks,
		unsigned char* data, const texture* textures, int offset)
{
	// copy the block colour into the pixel buffer
	unsigned char blockid = blocks[offset];
	unsigned char type = data[offset] % textures[blockid].mask;
	memcpy(pixel, &textures[blockid].types[type].colour, CHANNELS);

	// if block colour is not fully opaque, combine with the block below it
	if (pixel[ALPHA] < 255)
	{
		// get the next block down, or use black if this is the bottom block
		unsigned char next[CHANNELS] = {0};
		if (offset > CHUNK_BLOCK_AREA)
			get_block_alpha_colour(next, blocks, data, textures, offset - CHUNK_BLOCK_AREA);
		combine_alpha(pixel, next, 0);
	}
}


static void adjust_colour_by_height(unsigned char* pixel, int y)
{
	if (pixel[ALPHA] == 0) return;

	float alt = (float)y / MAX_HEIGHT - HEIGHT_MIDPOINT; // +/- distance from center
	float mod = alt / (alt < 0 ? HEIGHT_MIDPOINT : 1 - HEIGHT_MIDPOINT); // amount to adjust
	adjust_colour_brightness(pixel, mod * HEIGHT_CONTRAST);
}


int get_offset(const int y, const int x, const int z, const unsigned char rotate)
{
	int bx, bz;
	switch(rotate) {
	case 0:
		bx = x;
		bz = z;
		break;
	case 1:
		bx = z;
		bz = MAX_BLOCK - x;
		break;
	case 2:
		bx = MAX_BLOCK - x;
		bz = MAX_BLOCK - z;
		break;
	case 3:
		bx = MAX_BLOCK - z;
		bz = x;
		break;
	}
	return y * CHUNK_BLOCK_AREA + bz * CHUNK_BLOCK_LENGTH + bx;
}


image render_chunk_blockmap(nbt_node* chunk, const texture* textures, const char night,
		const char rotate, nbt_node* neighbours[4])
{
	image cimage = create_image(CHUNK_BLOCK_LENGTH, CHUNK_BLOCK_LENGTH);

	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* data = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* blight = NULL;
	if (night) blight = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	get_chunk_blockdata(chunk, blocks, data, blight);

	// get block data for neighbouring chunks, if they exist
	unsigned char* nblocks[4];
	for (int i = 0; i < 4; i++)
	{
		if (neighbours == NULL || neighbours[i] == NULL)
		{
			nblocks[i] = NULL;
			continue;
		}
		nblocks[i] = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
		get_chunk_blockdata(neighbours[i], nblocks[i], NULL, NULL);
	}

	// loop through rotated chunk's blocks
	for (int z = 0; z <= MAX_BLOCK; z++)
	{
		for (int x = 0; x <= MAX_BLOCK; x++)
		{
			// get pixel buffer for this block's rotated position
			unsigned char* pixel = &cimage.data[(z * CHUNK_BLOCK_LENGTH + x) * CHANNELS];

			// get unrotated 2d block offset from rotated coordinates
			int hoffset = get_offset(0, x, z, rotate);
			for (int y = MAX_HEIGHT; y >= 0; y--)
			{
				// get unrotated 3d block offset
				int offset = y * CHUNK_BLOCK_AREA + hoffset;

				unsigned char blockid = blocks[offset];
				if (blockid == 0 || blockid >= BLOCK_TYPES) continue;

				get_block_alpha_colour(pixel, blocks, data, textures, offset);
				adjust_colour_by_height(pixel, y);

				// highlights and shadows
				unsigned char nb[4] = {
						z > 0 ? blocks[get_offset(y, x, z - 1, rotate)] :
								(nblocks[0] == NULL ? 0 :
										nblocks[0][get_offset(y, x, MAX_BLOCK, rotate)]),

						x < MAX_BLOCK ? blocks[get_offset(y, x + 1, z, rotate)] :
								(nblocks[1] == NULL ? 0 :
										nblocks[1][get_offset(y, 0, z, rotate)]),

						z < MAX_BLOCK ? blocks[get_offset(y, x, z + 1, rotate)] :
								(nblocks[2] == NULL ? 0 :
										nblocks[2][get_offset(y, x, 0, rotate)]),

						x > 0 ? blocks[get_offset(y, x - 1, z, rotate)] :
								(nblocks[3] == NULL ? 0 :
										nblocks[3][get_offset(y, MAX_BLOCK, z, rotate)]),
				};
				char light = (nb[0] == 0 || nb[3] == 0);
				char dark = (nb[1] == 0 || nb[2] == 0);
				if (light && !dark) adjust_colour_brightness(pixel, EDGE_SHADING);
				if (dark && !light) adjust_colour_brightness(pixel, -EDGE_SHADING);

				// night mode
				if (blight != NULL)
				{
					set_colour_brightness(pixel,
							(float)blight[offset] / MAX_LIGHT, NIGHT_AMBIENCE);
				}

				break;
			}
		}
	}
	free(blocks);
	free(data);
	free(blight);
	for (int i = 0; i < 4; i++) free(nblocks[i]);
	return cimage;
}


image render_chunk_iso_blockmap(nbt_node* chunk, const texture* textures, const char night,
		const char rotate, nbt_node* neighbours[4])
{
	image cimage = create_image(ISO_CHUNK_WIDTH, ISO_CHUNK_HEIGHT);
	//printf("Rendering isometric image, %d x %d\n", cimage.width, cimage.height);

	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* data = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* blight = NULL;
	if (night)
	{
		blight = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	}
	get_chunk_blockdata(chunk, blocks, data, blight);

	// get block data for neighbouring chunks, if they exist
	unsigned char* nblocks[4];
	for (int i = 0; i < 4; i++)
	{
		if (neighbours == NULL || neighbours[i] == NULL)
		{
			nblocks[i] = NULL;
			continue;
		}
		nblocks[i] = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
		get_chunk_blockdata(neighbours[i], nblocks[i], NULL, NULL);
	}

	for (int z = 0; z <= MAX_BLOCK; z++)
	{
		for (int x = 0; x <= MAX_BLOCK; x++)
		{
			// get unrotated 2d block offset from rotated coordinates
			int hoffset = get_offset(0, x, z, rotate);
			for (int y = 0; y <= MAX_HEIGHT; y++)
			{
				// get unrotated 3d block offset
				int offset = y * CHUNK_BLOCK_AREA + hoffset;

				unsigned char blockid = blocks[offset];
				if (blockid == 0 || blockid >= BLOCK_TYPES) continue;

				unsigned char type = data[offset] % textures[blockid].mask;

				unsigned char colour[CHANNELS], lshadow[CHANNELS], rshadow[CHANNELS];
				memcpy(&colour, &textures[blockid].types[type].colour, CHANNELS);
				adjust_colour_by_height(colour, y);
				if (blight != NULL)
				{
					set_colour_brightness(colour, (float)blight[offset] / MAX_HEIGHT,
							NIGHT_AMBIENCE);
				}

				memcpy(&lshadow, &colour, CHANNELS);
				memcpy(&rshadow, &colour, CHANNELS);
				set_colour_brightness(lshadow, 0.75, 0);
				set_colour_brightness(rshadow, 0.5, 0);

				// translate orthographic to isometric coordinates
				int px = (x + MAX_BLOCK - z) * ISO_BLOCK_WIDTH / 2;
				int py = (x + z) * ISO_BLOCK_TOP_HEIGHT
						+ (MAX_HEIGHT - y) * ISO_BLOCK_DEPTH;
				//printf("Block %d,%d,%d rendering at pixel %d,%d\n", bx, bz, by, px, py);

				for (int sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
				{
					// before drawing the top of this block, check the block above
					if (sy < ISO_BLOCK_TOP_HEIGHT && y < MAX_HEIGHT)
					{
						// if the block above is the same type, don't draw the top of this one
						unsigned char tblockid = blocks[offset + CHUNK_BLOCK_AREA];
						if (tblockid == blockid) continue;

						// if the block above is opaque, don't draw the top of this one
						unsigned char ttype = data[offset + CHUNK_BLOCK_AREA] %
								textures[tblockid].mask;
						if (textures[tblockid].types[ttype].colour[ALPHA] == 255) continue;
					}

					for (int sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
					{
						if (textures[blockid].types[type].shape[sy * ISO_BLOCK_WIDTH + sx])
						{
							unsigned char* shade = colour;
							if (sy >= ISO_BLOCK_TOP_HEIGHT)
							{
								shade = sx < ISO_BLOCK_WIDTH / 2 ? lshadow : rshadow;
							}

							combine_alpha(shade,
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
	for (int i = 0; i < 4; i++) free(nblocks[i]);
	return cimage;
}


void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate)
{
	image cimage = isometric
			? render_chunk_iso_blockmap(chunk, textures, night, rotate, NULL)
			: render_chunk_blockmap(chunk, textures, night, rotate, NULL);

	printf("Saving image to %s ...\n", imagefile);
	save_image(cimage, imagefile);
	free_image(cimage);
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
	image cimage = create_image(CHUNK_BLOCK_LENGTH, CHUNK_BLOCK_LENGTH);
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
	save_image(cimage, imagefile);
	free_image(cimage);
}
