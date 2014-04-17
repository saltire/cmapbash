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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chunk.h"
#include "image.h"
#include "textures.h"


#define MAX_LIGHT 15
#define MAX_BLOCK (CHUNK_BLOCK_LENGTH - 1)
#define MAX_HEIGHT (CHUNK_BLOCK_HEIGHT - 1)

// configurable render options
#define HEIGHT_MIDPOINT 0.3 // midpoint of height shading gradient
#define HEIGHT_CONTRAST 0.7 // amount of contrast for height shading
#define NIGHT_AMBIENCE 0.2 // base light level for night renders
#define CONTOUR_SHADING 0.125 // amount of contrast for topographical highlights and shadows


typedef struct chunkdata {
	unsigned char *bids, *bdata, *blight, *nbids[4], *nbdata[4], *nblight[4];
} chunkdata;


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


static void get_chunk_data(nbt_node* chunk_nbt, unsigned char* bids, unsigned char* bdata,
		unsigned char* blight)
{

	nbt_node* sections = nbt_find_by_name(chunk_nbt, "Sections");
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

				copy_section_bytes(section->data, "Blocks", bids, y);
				if (bdata != NULL) copy_section_half_bytes(section->data, "Data", bdata, y);
				if (blight != NULL) copy_section_half_bytes(section->data, "BlockLight", blight, y);
			}
		}
	}
}


static void get_block_alpha_colour(unsigned char* pixel, unsigned char* blocks,
		unsigned char* data, const textures* tex, int offset)
{
	// copy the block colour into the pixel buffer
	memcpy(pixel, get_block_type(tex, blocks[offset], data[offset]).colour, CHANNELS);

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


static void get_neighbour_values(unsigned char* data, unsigned char* ndata[4],
		unsigned char nvalues[4], const int x, const int y, const int z, const char rotate)
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
		chunkdata chunk, const int x, const int z, const char rotate)
{
	// get unrotated 2d block offset from rotated coordinates
	int hoffset = get_offset(0, x, z, rotate);
	for (int y = 0; y <= MAX_HEIGHT; y++)
	{
		// get unrotated 3d block offset
		int offset = y * CHUNK_BLOCK_AREA + hoffset;

		unsigned char bid, bdata, blight, nbids[4], nbdata[4], nblight[4];
		bid = chunk.bids[offset];
		bdata = chunk.bdata[offset];

		// skip air blocks or invalid block ids
		if (bid == 0 || bid > tex->max_blockid) continue;

		// get neighbour block ids and data values
		get_neighbour_values(chunk.bids, chunk.nbids, nbids, x, y, z, rotate);
		get_neighbour_values(chunk.bdata, chunk.nbdata, nbdata, x, y, z, rotate);

		// get the type of this block and neighbouring blocks
		const blocktype btype = get_block_type(tex, bid, bdata);
		const blocktype tbtype = get_block_type(tex,
				chunk.bids[offset + CHUNK_BLOCK_AREA], chunk.bdata[offset + CHUNK_BLOCK_AREA]);
		const blocktype lbtype = get_block_type(tex, nbids[2], nbdata[2]);
		const blocktype rbtype = get_block_type(tex, nbids[1], nbdata[1]);

		// if the block above is the same type or opaque, don't draw the top of this one
		// opaque means alpha opacity is 255 and shape id is 0 (a solid square)
		char draw_top = !(y < MAX_HEIGHT && (
				(tbtype.id == btype.id && tbtype.subtype == btype.subtype) ||
				(tbtype.colour[ALPHA] == 255 && tbtype.shape.is_solid)));

		// if block in front of left or right side is solid and opaque, don't draw that side
		char draw_left = !(lbtype.shape.is_solid && lbtype.colour[ALPHA] == 255);
		char draw_right = !(rbtype.shape.is_solid && rbtype.colour[ALPHA] == 255);

		// skip this block if we aren't drawing any of the faces
		if (!draw_top && !draw_left && !draw_right) continue;

		// get block colour
		unsigned char colours[COLOURS][CHANNELS];
		memcpy(&colours[BLOCK], btype.colour, CHANNELS);
		adjust_colour_by_height(colours[BLOCK], y);

		// get highlight and shadow if the shape uses them and that side is not blocked
		if (btype.shape.has_hilight)
		{
			memcpy(&colours[HILIGHT], &colours[BLOCK], CHANNELS);
			if (!lbtype.shape.is_solid) adjust_colour_brightness(colours[HILIGHT], CONTOUR_SHADING);
		}
		if (btype.shape.has_shadow)
		{
			memcpy(&colours[SHADOW], &colours[BLOCK], CHANNELS);
			if (!rbtype.shape.is_solid) adjust_colour_brightness(colours[SHADOW], -CONTOUR_SHADING);
		}

		// night mode: darken colours according to block light
		if (chunk.blight != NULL)
		{
			get_neighbour_values(chunk.blight, chunk.nblight, nblight, x, y, z, rotate);
			float tblight = y < MAX_HEIGHT ? (float)chunk.blight[offset + CHUNK_BLOCK_AREA] : 0;
			if (tblight < MAX_LIGHT)
				set_colour_brightness(colours[BLOCK], tblight / MAX_LIGHT, NIGHT_AMBIENCE);
			if (btype.shape.has_hilight && nblight[2] < MAX_LIGHT)
				set_colour_brightness(
						colours[HILIGHT], (float)nblight[2] / MAX_LIGHT, NIGHT_AMBIENCE);
			if (btype.shape.has_shadow && nblight[1] < MAX_LIGHT)
				set_colour_brightness(
						colours[SHADOW], (float)nblight[1] / MAX_LIGHT, NIGHT_AMBIENCE);
		}

		// translate orthographic to isometric coordinates
		int px = cpx + (x + MAX_BLOCK - z) * ISO_BLOCK_WIDTH / 2;
		int py = cpy + (x + z) * ISO_BLOCK_TOP_HEIGHT + (MAX_HEIGHT - y) * ISO_BLOCK_DEPTH;

		// draw pixels
		for (int sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
		{
			if (sy < ISO_BLOCK_TOP_HEIGHT && !draw_top) continue;
			for (int sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
			{
				unsigned char pcolour = btype.shape.pixels[sy * ISO_BLOCK_WIDTH + sx];
				if (pcolour == BLANK) continue;
				{
					int p = (py + sy) * image->width + px + sx;

					if ((sy < ISO_BLOCK_TOP_HEIGHT && draw_top) ||
							(sx < ISO_BLOCK_WIDTH / 2 && draw_left) ||
							(sx >= ISO_BLOCK_WIDTH / 2 && draw_right))
						combine_alpha(colours[pcolour], &image->data[p * CHANNELS], 1);
				}
			}
		}
	}
}


static void render_ortho_block(image* image, const int cpx, const int cpy, const textures* tex,
		chunkdata chunk, const int x, const int z, const char rotate)
{
	// get pixel buffer for this block's rotated position
	unsigned char* pixel = &image->data[((cpy + z) * image->width + cpx + x) * CHANNELS];

	// get unrotated 2d block offset from rotated coordinates
	int hoffset = get_offset(0, x, z, rotate);

	for (int y = MAX_HEIGHT; y >= 0; y--)
	{
		// get unrotated 3d block offset
		int offset = y * CHUNK_BLOCK_AREA + hoffset;

		// skip air blocks or invalid block ids
		if (chunk.bids[offset] == 0 || chunk.bids[offset] >= tex->max_blockid) continue;

		// get block colour
		get_block_alpha_colour(pixel, chunk.bids, chunk.bdata, tex, offset);
		adjust_colour_by_height(pixel, y);

		// contour highlights and shadows
		unsigned char nbids[4];
		get_neighbour_values(chunk.bids, chunk.nbids, nbids, x, y, z, rotate);
		char light = (nbids[0] == 0 || nbids[3] == 0);
		char dark = (nbids[1] == 0 || nbids[2] == 0);
		if (light && !dark) adjust_colour_brightness(pixel, CONTOUR_SHADING);
		if (dark && !light) adjust_colour_brightness(pixel, -CONTOUR_SHADING);

		// night mode: darken colours according to block light
		if (chunk.blight != NULL) {
			float tbl = y < MAX_HEIGHT ? chunk.blight[offset + CHUNK_BLOCK_AREA] : 0;
			if (tbl < MAX_LIGHT) set_colour_brightness(pixel, tbl / MAX_LIGHT, NIGHT_AMBIENCE);
		}

		break;
	}
}


void render_chunk_map(image* image, const int cpx, const int cpy,
		nbt_node* chunk_nbt, nbt_node* nchunks_nbt[4], const textures* tex,
		const char night, const char isometric, const char rotate)
{
	// get block data for this chunk
	chunkdata chunk;
	chunk.bids = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	chunk.bdata = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	chunk.blight = night ? (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char)) : NULL;
	get_chunk_data(chunk_nbt, chunk.bids, chunk.bdata, chunk.blight);

	for (int i = 0; i < 4; i++)
	{
		if (nchunks_nbt[i] == NULL)
		{
			chunk.nbids[i] = NULL;
			chunk.nbdata[i] = NULL;
			chunk.nblight[i] = NULL;
		}
		else
		{
			chunk.nbids[i] = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
			chunk.nbdata[i] = isometric ?
					(unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char)) : NULL;
			chunk.nblight[i] = isometric && night ?
					(unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char)) : NULL;
			get_chunk_data(nchunks_nbt[i], chunk.nbids[i], chunk.nbdata[i], chunk.nblight[i]);
		}
	}

	// loop through rotated chunk's blocks
	for (int z = 0; z <= MAX_BLOCK; z++)
	{
		for (int x = 0; x <= MAX_BLOCK; x++)
		{
			if (isometric)
				render_iso_column(image, cpx, cpy, tex, chunk, x, z, rotate);
			else
				render_ortho_block(image, cpx, cpy, tex, chunk, x, z, rotate);
		}
	}

	free(chunk.bids);
	free(chunk.bdata);
	free(chunk.blight);
	for (int i = 0; i < 4; i++)
	{
		free(chunk.nbids[i]);
		free(chunk.nbdata[i]);
		free(chunk.nblight[i]);
	}
}


void save_chunk_map(nbt_node* chunk, const char* imagefile, const textures* tex,
		const char night, const char isometric, const char rotate)
{
	image cimage = isometric ?
			create_image(ISO_CHUNK_WIDTH, ISO_CHUNK_HEIGHT) :
			create_image(CHUNK_BLOCK_LENGTH, CHUNK_BLOCK_LENGTH);

	clock_t start = clock();
	render_chunk_map(&cimage, 0, 0, chunk, NULL, tex, night, isometric, rotate);
	clock_t render_end = clock();
	printf("Total render time: %f seconds\n", (double)(render_end - start) / CLOCKS_PER_SEC);

	printf("Saving image to %s ...\n", imagefile);
	save_image(cimage, imagefile);
	printf("Total save time: %f seconds\n", (double)(clock() - render_end) / CLOCKS_PER_SEC);

	free_image(cimage);
}
