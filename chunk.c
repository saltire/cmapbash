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
#define CONTOUR_SHADING 0.125 // amoutn of contrast for topographical highlights and shadows


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


static void get_chunk_data(nbt_node* chunk, unsigned char* blocks, unsigned char* data,
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


static void get_neighbour_chunk_data(nbt_node* neighbours[4], unsigned char* nblocks[4])
{
	for (int i = 0; i < 4; i++)
	{
		if (neighbours == NULL || neighbours[i] == NULL)
		{
			nblocks[i] = NULL;
			continue;
		}
		nblocks[i] = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
		get_chunk_data(neighbours[i], nblocks[i], NULL, NULL);
	}
}


static void get_block_alpha_colour(unsigned char* pixel, unsigned char* blocks,
		unsigned char* data, const textures* tex, int offset)
{
	// copy the block colour into the pixel buffer
	memcpy(pixel, get_block_colour(tex, blocks[offset], data[offset]), CHANNELS);

	// if block colour is not fully opaque, combine with the block below it
	if (pixel[ALPHA] < 255)
	{
		// get the next block down, or use black if this is the bottom block
		unsigned char next[CHANNELS] = {0};
		if (offset > CHUNK_BLOCK_AREA)
			get_block_alpha_colour(next, blocks, data, tex, offset - CHUNK_BLOCK_AREA);
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


static int get_offset(const int y, const int x, const int z, const unsigned char rotate)
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


static void get_neighbour_values(unsigned char nvalues[4], unsigned char* data,
		unsigned char* ndata[4], const int x, const int y, const int z, const char rotate)
{
	nvalues[0] = z > 0 ? data[get_offset(y, x, z - 1, rotate)] :
			(ndata[0] == NULL ? 0 : ndata[0][get_offset(y, x, MAX_BLOCK, rotate)]);

	nvalues[1] = x < MAX_BLOCK ? data[get_offset(y, x + 1, z, rotate)] :
			(ndata[1] == NULL ? 0 : ndata[1][get_offset(y, 0, z, rotate)]);

	nvalues[2] = z < MAX_BLOCK ? data[get_offset(y, x, z + 1, rotate)] :
			(ndata[2] == NULL ? 0 : ndata[2][get_offset(y, x, 0, rotate)]);

	nvalues[3] = x > 0 ? data[get_offset(y, x - 1, z, rotate)] :
			(ndata[3] == NULL ? 0 : ndata[3][get_offset(y, MAX_BLOCK, z, rotate)]);
}


static void render_iso_column(image* image, const int cpx, const int cpy, const textures* tex,
		unsigned char* blocks, unsigned char* data, unsigned char* blight,
		unsigned char* nblocks[4], unsigned char* ndata[4],
		const int x, const int z, const char rotate)
{
	// get unrotated 2d block offset from rotated coordinates
	int hoffset = get_offset(0, x, z, rotate);
	for (int y = 0; y <= MAX_HEIGHT; y++)
	{
		// get unrotated 3d block offset
		int offset = y * CHUNK_BLOCK_AREA + hoffset;

		// skip air blocks or invalid block ids
		unsigned char blockid = blocks[offset];
		if (blockid == 0 || blockid >= tex->blockcount) continue;

		unsigned char dataval = data[offset];
		// get neighbour block ids and data values
		unsigned char nb[4], nd[4];
		get_neighbour_values(nb, blocks, nblocks, x, y, z, rotate);
		get_neighbour_values(nd, data, ndata, x, y, z, rotate);

		// get block colour
		unsigned char tcolour[CHANNELS], lcolour[CHANNELS], rcolour[CHANNELS];
		memcpy(&tcolour, get_block_colour(tex, blockid, dataval), CHANNELS);
		adjust_colour_by_height(tcolour, y);
		// copy the top colour to the left and right side colours
		memcpy(&lcolour, &tcolour, CHANNELS);
		memcpy(&rcolour, &tcolour, CHANNELS);

		// highlights and shadows on left and right sides
		if (nb[2] == 0) adjust_colour_brightness(lcolour, CONTOUR_SHADING);
		if (nb[1] == 0) adjust_colour_brightness(rcolour, -CONTOUR_SHADING);
		// night mode
		if (blight != NULL && blight[offset] < MAX_LIGHT)
		{
			set_colour_brightness(tcolour, (float)blight[offset] / MAX_HEIGHT, NIGHT_AMBIENCE);
			set_colour_brightness(lcolour, (float)blight[offset] / MAX_HEIGHT, NIGHT_AMBIENCE);
			set_colour_brightness(rcolour, (float)blight[offset] / MAX_HEIGHT, NIGHT_AMBIENCE);
		}

		char draw_top = 1;
		char draw_left = 1;
		char draw_right = 1;
		// if the block above is the same type or opaque, don't draw the top of this one
		// opaque means alpha opacity is 255 and shape id is 0 (a solid square)
		if (y < MAX_HEIGHT)
		{
			unsigned char tblock = blocks[offset + CHUNK_BLOCK_AREA];
			if (tblock == blockid) draw_top = 0;
			else
			{
				unsigned char tdata = data[offset + CHUNK_BLOCK_AREA];
				if (get_block_colour(tex, tblock, tdata)[ALPHA] == 255 &&
						get_block_shapeid(tex, tblock, tdata) == 0) draw_top = 0;
			}
		}
		// if block in front of left or right side is opaque, don't draw that side
		if (get_block_colour(tex, nb[2], nd[2])[ALPHA] == 255 &&
				get_block_shapeid(tex, nb[2], nd[2]) == 0) draw_left = 0;
		if (get_block_colour(tex, nb[1], nd[1])[ALPHA] == 255 &&
				get_block_shapeid(tex, nb[1], nd[1]) == 0) draw_right = 0;

		// translate orthographic to isometric coordinates
		int px = cpx + (x + MAX_BLOCK - z) * ISO_BLOCK_WIDTH / 2;
		int py = cpy + (x + z) * ISO_BLOCK_TOP_HEIGHT + (MAX_HEIGHT - y) * ISO_BLOCK_DEPTH;

		const unsigned char shapeid = get_block_shapeid(tex, blockid, dataval);
		for (int sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
		{
			if (sy < ISO_BLOCK_TOP_HEIGHT && !draw_top) continue;
			for (int sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
			{
				if (tex->shapes[shapeid * ISO_BLOCK_AREA + sy * ISO_BLOCK_WIDTH + sx])
				{
					int p = (py + sy) * image->width + px + sx;
					if (sy < ISO_BLOCK_TOP_HEIGHT)
						combine_alpha(tcolour, &image->data[p * CHANNELS], 1);
					else if (sx < ISO_BLOCK_WIDTH / 2 && draw_left)
						combine_alpha(shapeid ? tcolour : lcolour, &image->data[p * CHANNELS], 1);
					else if (sx >= ISO_BLOCK_WIDTH / 2 && draw_right)
						combine_alpha(shapeid ? tcolour : rcolour, &image->data[p * CHANNELS], 1);
				}
			}
		}
	}
}


static void render_ortho_block(image* image, const int cpx, const int cpy, const textures* tex,
		unsigned char* blocks, unsigned char* data, unsigned char* blight,
		unsigned char* nblocks[4], const int x, const int z, const char rotate)
{
	// get pixel buffer for this block's rotated position
	unsigned char* pixel = &image->data[((cpy + z) * image->width + cpx + x) * CHANNELS];

	// get unrotated 2d block offset from rotated coordinates
	int hoffset = get_offset(0, x, z, rotate);

	for (int y = MAX_HEIGHT; y >= 0; y--)
	{
		// get unrotated 3d block offset
		int offset = y * CHUNK_BLOCK_AREA + hoffset;

		unsigned char blockid = blocks[offset];
		if (blockid == 0 || blockid >= tex->blockcount) continue;

		get_block_alpha_colour(pixel, blocks, data, tex, offset);
		adjust_colour_by_height(pixel, y);

		// highlights and shadows
		unsigned char nb[4];
		get_neighbour_values(nb, blocks, nblocks, x, y, z, rotate);
		char light = (nb[0] == 0 || nb[3] == 0);
		char dark = (nb[1] == 0 || nb[2] == 0);
		if (light && !dark) adjust_colour_brightness(pixel, CONTOUR_SHADING);
		if (dark && !light) adjust_colour_brightness(pixel, -CONTOUR_SHADING);

		// night mode
		if (blight != NULL && blight[offset] < MAX_LIGHT)
			set_colour_brightness(pixel, (float)blight[offset] / MAX_LIGHT, NIGHT_AMBIENCE);

		break;
	}
}


void render_chunk_map(image* image, const int cpx, const int cpy,
		nbt_node* chunk, nbt_node* nchunks[4], const textures* tex,
		const char night, const char isometric, const char rotate)
{

	// get block data for this chunk
	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* data = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* blight = night ? (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char)) : NULL;
	get_chunk_data(chunk, blocks, data, blight);

	// get block data for neighbouring chunks, if they exist
	unsigned char *nblocks[4], *ndata[4];
	get_neighbour_chunk_data(nchunks, nblocks);
	if (isometric) get_neighbour_chunk_data(nchunks, ndata);

	// loop through rotated chunk's blocks
	for (int z = 0; z <= MAX_BLOCK; z++)
	{
		for (int x = 0; x <= MAX_BLOCK; x++)
		{
			if (isometric)
				render_iso_column(image, cpx, cpy, tex, blocks, data, blight, nblocks, ndata,
						x, z, rotate);
			else
				render_ortho_block(image, cpx, cpy, tex, blocks, data, blight, nblocks,
						x, z, rotate);
		}
	}

	free(blocks);
	free(data);
	free(blight);
	for (int i = 0; i < 4; i++)
	{
		free(nblocks[i]);
		if (isometric) free(ndata[i]);
	}
}


void save_chunk_map(nbt_node* chunk, const char* imagefile, const textures* tex,
		const char night, const char isometric, const char rotate)
{
	image cimage = isometric ?
			create_image(ISO_CHUNK_WIDTH, ISO_CHUNK_HEIGHT) :
			create_image(CHUNK_BLOCK_LENGTH, CHUNK_BLOCK_LENGTH);
	render_chunk_map(&cimage, 0, 0, chunk, NULL, tex, night, isometric, rotate);

	printf("Saving image to %s ...\n", imagefile);
	save_image(cimage, imagefile);
	free_image(cimage);
}
