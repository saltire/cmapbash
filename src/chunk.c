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
#include "dims.h"
#include "image.h"
#include "textures.h"


// configurable render options
#define HSHADE_HEIGHT 0.3 // height below which to add shadows
#define HSHADE_AMOUNT 0.7 // amount of shadow to add
#define NIGHT_AMBIENCE 0.2 // base light level for night renders

#define HSHADE_BLOCK_HEIGHT (HSHADE_HEIGHT * MAX_HEIGHT)


typedef struct chunkdata
{
	unsigned char *bids, *bdata, *blight, *slight, *nbids[4], *nbdata[4], *nblight[4], *nslight[4];
}
chunkdata;


static void copy_section_bytes(nbt_node *section, char *name, unsigned char *data, int8_t y)
{
	nbt_node *array = nbt_find_by_name(section, name);
	if (array->type != TAG_BYTE_ARRAY ||
			array->payload.tag_byte_array.length != SECTION_BLOCK_VOLUME)
	{
		fprintf(stderr, "Problem parsing section byte data.\n");
	}
	memcpy(data + y * SECTION_BLOCK_VOLUME, array->payload.tag_byte_array.data,
			SECTION_BLOCK_VOLUME);
}


static void copy_section_half_bytes(nbt_node *section, char *name, unsigned char *data,
		int8_t y)
{
	nbt_node *array = nbt_find_by_name(section, name);
	if (array->type != TAG_BYTE_ARRAY ||
			array->payload.tag_byte_array.length != SECTION_BLOCK_VOLUME / 2)
	{
		fprintf(stderr, "Problem parsing section byte data.\n");
	}
	for (int i = 0; i < SECTION_BLOCK_VOLUME; i += 2)
	{
		unsigned char byte = array->payload.tag_byte_array.data[i / 2];
		data[y * SECTION_BLOCK_VOLUME + i] = byte % 16;
		data[y * SECTION_BLOCK_VOLUME + i + 1] = byte / 16;
	}
}


static void get_chunk_data(nbt_node *chunk_nbt, unsigned char *bids, unsigned char *bdata,
		unsigned char *blight, unsigned char *slight)
{
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
				}
				int8_t y = ynode->payload.tag_byte;

				copy_section_bytes(section->data, "Blocks", bids, y);
				if (bdata != NULL) copy_section_half_bytes(section->data, "Data", bdata, y);
				if (blight != NULL) copy_section_half_bytes(section->data, "BlockLight", blight, y);
				if (slight != NULL) copy_section_half_bytes(section->data, "SkyLight", slight, y);
			}
		}
	}
}


static void get_block_alpha_colour(unsigned char *pixel, unsigned char *blocks,
		unsigned char *data, const textures *tex, int offset)
{
	// copy the block colour into the pixel buffer
	memcpy(pixel, get_block_type(tex, blocks[offset], data[offset])->colours[COLOUR1], CHANNELS);

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


static void add_height_shading(unsigned char *pixel, int y)
{
	if (pixel[ALPHA] == 0 || y >= HSHADE_BLOCK_HEIGHT) return;
	adjust_colour_brightness(pixel, (((float)y / HSHADE_BLOCK_HEIGHT) - 1) * HSHADE_AMOUNT);
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
		bz = MAX_CHUNK_BLOCK - x;
		break;
	case 2:
		bx = MAX_CHUNK_BLOCK - x;
		bz = MAX_CHUNK_BLOCK - z;
		break;
	case 3:
		bx = MAX_CHUNK_BLOCK - z;
		bz = x;
		break;
	}
	return y * CHUNK_BLOCK_AREA + bz * CHUNK_BLOCK_LENGTH + bx;
}


static void get_neighbour_values(unsigned char *data, unsigned char *ndata[4],
		unsigned char nvalues[4], const int x, const int y, const int z, const char rotate)
{
	nvalues[0] = z > 0 ? data[get_offset(y, x, z - 1, rotate)] :
			(ndata[0] == NULL ? 0 : ndata[0][get_offset(y, x, MAX_CHUNK_BLOCK, rotate)]);

	nvalues[1] = x < MAX_CHUNK_BLOCK ? data[get_offset(y, x + 1, z, rotate)] :
			(ndata[1] == NULL ? 0 : ndata[1][get_offset(y, 0, z, rotate)]);

	nvalues[2] = z < MAX_CHUNK_BLOCK ? data[get_offset(y, x, z + 1, rotate)] :
			(ndata[2] == NULL ? 0 : ndata[2][get_offset(y, x, 0, rotate)]);

	nvalues[3] = x > 0 ? data[get_offset(y, x - 1, z, rotate)] :
			(ndata[3] == NULL ? 0 : ndata[3][get_offset(y, MAX_CHUNK_BLOCK, z, rotate)]);
}


static void render_iso_column(image *image, const int cpx, const int cpy, const textures *tex,
		chunkdata chunk, const int rbx, const int rbz, const char rotate)
{
	// get unrotated 2d block offset from rotated coordinates
	int hoffset = get_offset(0, rbx, rbz, rotate);
	for (int y = 0; y <= MAX_HEIGHT; y++)
	{
		// get unrotated 3d block offset
		int offset = y * CHUNK_BLOCK_AREA + hoffset;

		unsigned char bid, bdata, blight, nbids[4], nbdata[4], nblight[4], nslight[4];
		bid = chunk.bids[offset];
		bdata = chunk.bdata[offset];

		// skip air blocks or invalid block ids
		if (bid == 0 || bid > tex->max_blockid) continue;

		// get neighbour block ids and data values
		get_neighbour_values(chunk.bids, chunk.nbids, nbids, rbx, y, rbz, rotate);
		get_neighbour_values(chunk.bdata, chunk.nbdata, nbdata, rbx, y, rbz, rotate);

		// get the type of this block and neighbouring blocks
		const blocktype *btype = get_block_type(tex, bid, bdata);
		const blocktype *tbtype = y == MAX_HEIGHT ? NULL : get_block_type(tex,
				chunk.bids[offset + CHUNK_BLOCK_AREA], chunk.bdata[offset + CHUNK_BLOCK_AREA]);
		const blocktype *lbtype = get_block_type(tex, nbids[2], nbdata[2]);
		const blocktype *rbtype = get_block_type(tex, nbids[1], nbdata[1]);

		// if the block above is the same type or opaque, don't draw the top of this one
		char draw_top = !(y < MAX_HEIGHT && (
				(tbtype->id == btype->id && tbtype->subtype == btype->subtype) ||
				(tbtype->shapes[rotate].is_solid && tbtype->is_opaque)));

		// if block in front of left or right side is solid and opaque, don't draw that side
		char draw_left = !(lbtype->shapes[rotate].is_solid && lbtype->is_opaque);
		char draw_right = !(rbtype->shapes[rotate].is_solid && rbtype->is_opaque);

		// skip this block if we aren't drawing any of the faces
		if (!draw_top && !draw_left && !draw_right) continue;

		// get block shape for this rotation
		shape bshape = btype->shapes[rotate];

		// get block colours and adjust for height
		unsigned char colours[COLOUR_COUNT][CHANNELS];
		memcpy(&colours, btype->colours, COLOUR_COUNT * CHANNELS);
		for (int i = 0; i < COLOUR_COUNT; i++)
			if (bshape.has[i]) add_height_shading(colours[i], y);

		// replace highlight and/or shadow with unshaded colour if that side is blocked
		if (lbtype->shapes[rotate].is_solid)
		{
			if (bshape.has[HILIGHT1]) memcpy(&colours[HILIGHT1], &colours[COLOUR1], CHANNELS);
			if (bshape.has[HILIGHT2]) memcpy(&colours[HILIGHT2], &colours[COLOUR2], CHANNELS);
		}
		if (rbtype->shapes[rotate].is_solid)
		{
			if (bshape.has[SHADOW1]) memcpy(&colours[SHADOW1], &colours[COLOUR1], CHANNELS);
			if (bshape.has[SHADOW2]) memcpy(&colours[SHADOW2], &colours[COLOUR2], CHANNELS);
		}

		// darken colours according to sky light (day) or block light (night)
		unsigned char *clight = chunk.slight != NULL ? chunk.slight :
				(chunk.blight != NULL ? chunk.blight : NULL);
		if (clight != NULL)
		{
			unsigned char nclight[4];
			get_neighbour_values(clight, (chunk.slight != NULL ? chunk.nslight :
					(chunk.blight != NULL ? chunk.nblight : NULL)), nclight, rbx, y, rbz, rotate);

			float tclight = y == MAX_HEIGHT ? 0 : (float)clight[offset + CHUNK_BLOCK_AREA];
			if (tclight < MAX_LIGHT)
			{
				tclight /= MAX_LIGHT;
				set_colour_brightness(colours[COLOUR1], tclight, NIGHT_AMBIENCE);
				if (bshape.has[COLOUR2])
					set_colour_brightness(colours[COLOUR2], tclight, NIGHT_AMBIENCE);
			}
			if (draw_left && nclight[2] < MAX_LIGHT)
			{
				float lclight = (float)nclight[2] / MAX_LIGHT;
				if (bshape.has[HILIGHT1])
					set_colour_brightness(colours[HILIGHT1], lclight, NIGHT_AMBIENCE);
				if (bshape.has[HILIGHT2])
					set_colour_brightness(colours[HILIGHT2], lclight, NIGHT_AMBIENCE);
			}
			if (draw_right && nclight[1] < MAX_LIGHT)
			{
				float rclight = (float)nclight[1] / MAX_LIGHT;
				if (bshape.has[SHADOW1])
					set_colour_brightness(colours[SHADOW1], rclight, NIGHT_AMBIENCE);
				if (bshape.has[SHADOW2])
					set_colour_brightness(colours[SHADOW2], rclight, NIGHT_AMBIENCE);
			}
		}

		// translate orthographic to isometric coordinates
		int px = cpx + (rbx + MAX_CHUNK_BLOCK - rbz) * ISO_BLOCK_WIDTH / 2;
		int py = cpy + (rbx + rbz) * ISO_BLOCK_TOP_HEIGHT + (MAX_HEIGHT - y) * ISO_BLOCK_DEPTH;

		// draw pixels
		for (int sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
		{
			if (sy < ISO_BLOCK_TOP_HEIGHT && !draw_top) continue;
			for (int sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
			{
				unsigned char pcolour = bshape.pixels[sy * ISO_BLOCK_WIDTH + sx];
				if (pcolour == BLANK) continue;
				{
					int po = (py + sy) * image->width + px + sx;

					if ((sy < ISO_BLOCK_TOP_HEIGHT) ||
							(sx < ISO_BLOCK_WIDTH / 2 && draw_left) ||
							(sx >= ISO_BLOCK_WIDTH / 2 && draw_right))
						combine_alpha(colours[pcolour], &image->data[po * CHANNELS], 1);
				}
			}
		}
	}
}


static void render_ortho_block(image *image, const int cpx, const int cpy, const textures *tex,
		chunkdata chunk, const int rbx, const int rbz, const char rotate)
{
	// get pixel buffer for this block's rotated position
	unsigned char *pixel = &image->data[((cpy + rbz) * image->width + cpx + rbx) * CHANNELS];

	// get unrotated 2d block offset from rotated coordinates
	int hoffset = get_offset(0, rbx, rbz, rotate);

	for (int y = MAX_HEIGHT; y >= 0; y--)
	{
		// get unrotated 3d block offset
		int offset = y * CHUNK_BLOCK_AREA + hoffset;

		// skip air blocks or invalid block ids
		if (chunk.bids[offset] == 0 || chunk.bids[offset] >= tex->max_blockid) continue;

		// get block colour
		get_block_alpha_colour(pixel, chunk.bids, chunk.bdata, tex, offset);
		add_height_shading(pixel, y);

		// contour highlights and shadows
		unsigned char nbids[4];
		get_neighbour_values(chunk.bids, chunk.nbids, nbids, rbx, y, rbz, rotate);
		char light = (nbids[0] == 0 || nbids[3] == 0);
		char dark = (nbids[1] == 0 || nbids[2] == 0);
		if (light && !dark) adjust_colour_brightness(pixel, HILIGHT_AMOUNT);
		if (dark && !light) adjust_colour_brightness(pixel, SHADOW_AMOUNT);

		// night mode: darken colours according to block light
		if (chunk.blight != NULL) {
			float tbl = y < MAX_HEIGHT ? chunk.blight[offset + CHUNK_BLOCK_AREA] : 0;
			if (tbl < MAX_LIGHT) set_colour_brightness(pixel, tbl / MAX_LIGHT, NIGHT_AMBIENCE);
		}

		break;
	}
}


void render_chunk_map(image *image, const int cpx, const int cpy,
		nbt_node *chunk_nbt, nbt_node *nchunks_nbt[4], const textures *tex, const options opts)
{
	// get block data for this chunk
	chunkdata chunk;
	chunk.bids = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	chunk.bdata = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	chunk.blight = opts.night ? (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char)) : NULL;
	chunk.slight = opts.isometric && !opts.night && opts.shadows ?
			(unsigned char*)malloc(CHUNK_BLOCK_VOLUME) : NULL;
	// initialize skylight to full rather than zero
	if (chunk.slight != NULL) memset(chunk.slight, 255, CHUNK_BLOCK_VOLUME);
	get_chunk_data(chunk_nbt, chunk.bids, chunk.bdata, chunk.blight, chunk.slight);

	// get block data for neighbouring chunks
	for (int i = 0; i < 4; i++)
	{
		if (nchunks_nbt[i] == NULL)
		{
			chunk.nbids[i] = NULL;
			chunk.nbdata[i] = NULL;
			chunk.nblight[i] = NULL;
			chunk.nslight[i] = NULL;
		}
		else
		{
			chunk.nbids[i] = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
			chunk.nbdata[i] = opts.isometric ?
					(unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char)) : NULL;
			chunk.nblight[i] = opts.isometric && opts.night ?
					(unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char)) : NULL;
			chunk.nslight[i] = opts.isometric && !opts.night && opts.shadows ?
					(unsigned char*)malloc(CHUNK_BLOCK_VOLUME) : NULL;
			if (chunk.nslight[i] != NULL) memset(chunk.nslight[i], 255, CHUNK_BLOCK_VOLUME);
			get_chunk_data(nchunks_nbt[i], chunk.nbids[i], chunk.nbdata[i], chunk.nblight[i],
					chunk.nslight[i]);
		}
	}

	// loop through rotated chunk's blocks
	for (int rbz = 0; rbz <= MAX_CHUNK_BLOCK; rbz++)
	{
		for (int rbx = 0; rbx <= MAX_CHUNK_BLOCK; rbx++)
		{
			if (opts.isometric)
				render_iso_column(image, cpx, cpy, tex, chunk, rbx, rbz, opts.rotate);
			else
				render_ortho_block(image, cpx, cpy, tex, chunk, rbx, rbz, opts.rotate);
		}
	}

	free(chunk.bids);
	free(chunk.bdata);
	free(chunk.blight);
	free(chunk.slight);
	for (int i = 0; i < 4; i++)
	{
		free(chunk.nbids[i]);
		free(chunk.nbdata[i]);
		free(chunk.nblight[i]);
		free(chunk.nslight[i]);
	}
}


void save_chunk_map(nbt_node *chunk, const char *imagefile, const options opts)
{
	image cimage = opts.isometric ?
			create_image(ISO_CHUNK_WIDTH, ISO_CHUNK_HEIGHT) :
			create_image(CHUNK_BLOCK_LENGTH, CHUNK_BLOCK_LENGTH);

	textures *tex = read_textures(opts.texpath, opts.shapepath);

	clock_t start = clock();
	render_chunk_map(&cimage, 0, 0, chunk, NULL, tex, opts);
	clock_t render_end = clock();
	printf("Total render time: %f seconds\n", (double)(render_end - start) / CLOCKS_PER_SEC);

	free_textures(tex);

	printf("Saving image to %s ...\n", imagefile);
	save_image(cimage, imagefile);
	printf("Total save time: %f seconds\n", (double)(clock() - render_end) / CLOCKS_PER_SEC);

	free_image(cimage);
}
