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

#include "chunkdata.h"
#include "chunkmap.h"
#include "dims.h"
#include "image.h"
#include "textures.h"


// configurable render options
#define HSHADE_HEIGHT 0.3 // height below which to add shadows
#define HSHADE_AMOUNT 0.7 // amount of shadow to add
#define NIGHT_AMBIENCE 0.2 // base light level for night renders

#define HSHADE_BLOCK_HEIGHT (HSHADE_HEIGHT * MAX_HEIGHT)


static int get_offset(const unsigned int y, const unsigned int rbx, const unsigned int rbz,
		const unsigned char rotate)
{
	int bx, bz;
	switch(rotate) {
	case 0:
		bx = rbx;
		bz = rbz;
		break;
	case 1:
		bx = rbz;
		bz = MAX_CHUNK_BLOCK - rbx;
		break;
	case 2:
		bx = MAX_CHUNK_BLOCK - rbx;
		bz = MAX_CHUNK_BLOCK - rbz;
		break;
	case 3:
		bx = MAX_CHUNK_BLOCK - rbz;
		bz = rbx;
		break;
	}
	return y * CHUNK_BLOCK_AREA + bz * CHUNK_BLOCK_LENGTH + bx;
}


static void get_neighbour_values(unsigned char *data, unsigned char *ndata[4],
		unsigned char nvalues[4], const int rbx, const int y, const int rbz, const char rotate,
		unsigned char defval)
{
	nvalues[0] = rbz > 0 ? data[get_offset(y, rbx, rbz - 1, rotate)] :
			(ndata[0] == NULL ? defval : ndata[0][get_offset(y, rbx, MAX_CHUNK_BLOCK, rotate)]);

	nvalues[1] = rbx < MAX_CHUNK_BLOCK ? data[get_offset(y, rbx + 1, rbz, rotate)] :
			(ndata[1] == NULL ? defval : ndata[1][get_offset(y, 0, rbz, rotate)]);

	nvalues[2] = rbz < MAX_CHUNK_BLOCK ? data[get_offset(y, rbx, rbz + 1, rotate)] :
			(ndata[2] == NULL ? defval : ndata[2][get_offset(y, rbx, 0, rotate)]);

	nvalues[3] = rbx > 0 ? data[get_offset(y, rbx - 1, rbz, rotate)] :
			(ndata[3] == NULL ? defval : ndata[3][get_offset(y, MAX_CHUNK_BLOCK, rbz, rotate)]);
}


static void add_height_shading(unsigned char *pixel, const unsigned int y)
{
	if (pixel[ALPHA] == 0 || y >= HSHADE_BLOCK_HEIGHT) return;
	adjust_colour_brightness(pixel, (((float)y / HSHADE_BLOCK_HEIGHT) - 1) * HSHADE_AMOUNT);
}


static void set_light_levels(unsigned char colours[COLOUR_COUNT][CHANNELS], const shape *bshape,
		const unsigned int offset, const unsigned char *clight, const unsigned char nlight[],
		const char draw_t, const char draw_l, const char draw_r, const unsigned char defval)
{
	int toffset = offset + CHUNK_BLOCK_AREA;
	float tlight = toffset > CHUNK_BLOCK_VOLUME ? defval : (float)clight[toffset];
	if (draw_t && tlight < MAX_LIGHT)
	{
		tlight /= MAX_LIGHT;
		set_colour_brightness(colours[COLOUR1], tlight, NIGHT_AMBIENCE);
		if (bshape->has[COLOUR2])
			set_colour_brightness(colours[COLOUR2], tlight, NIGHT_AMBIENCE);
	}
	if (draw_l && nlight[2] < MAX_LIGHT)
	{
		float llight = (float)nlight[2] / MAX_LIGHT;
		if (bshape->has[HILIGHT1])
			set_colour_brightness(colours[HILIGHT1], llight, NIGHT_AMBIENCE);
		if (bshape->has[HILIGHT2])
			set_colour_brightness(colours[HILIGHT2], llight, NIGHT_AMBIENCE);
	}
	if (draw_r && nlight[1] < MAX_LIGHT)
	{
		float rlight = (float)nlight[1] / MAX_LIGHT;
		if (bshape->has[SHADOW1])
			set_colour_brightness(colours[SHADOW1], rlight, NIGHT_AMBIENCE);
		if (bshape->has[SHADOW2])
			set_colour_brightness(colours[SHADOW2], rlight, NIGHT_AMBIENCE);
	}

}


static void render_iso_column(image *img, const int cpx, const int cpy, const textures *tex,
		chunkdata chunk, const unsigned int rbx, const unsigned int rbz, const char rotate)
{
	// get unrotated 2d block offset from rotated coordinates
	unsigned int hoffset = get_offset(0, rbx, rbz, rotate);
	for (unsigned int y = 0; y <= MAX_HEIGHT; y++)
	{
		// get unrotated 3d block offset
		unsigned int offset = y * CHUNK_BLOCK_AREA + hoffset;

		unsigned char bid, bdata, blight, nbids[4], nbdata[4], nblight[4], nslight[4];
		bid = chunk.bids[offset];
		bdata = chunk.bdata[offset];

		// skip air blocks or invalid block ids
		if (bid == 0 || bid > tex->max_blockid) continue;

		// get neighbour block ids and data values
		get_neighbour_values(chunk.bids, chunk.nbids, nbids, rbx, y, rbz, rotate, 0);
		get_neighbour_values(chunk.bdata, chunk.nbdata, nbdata, rbx, y, rbz, rotate, 0);

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
		const shape *bshape = &btype->shapes[rotate];

		// get block colours and adjust for height
		unsigned char colours[COLOUR_COUNT][CHANNELS];
		memcpy(&colours, btype->colours, COLOUR_COUNT * CHANNELS);
		for (int i = 0; i < COLOUR_COUNT; i++)
			if (bshape->has[i]) add_height_shading(colours[i], y);

		// replace highlight and/or shadow with unshaded colour if that side is blocked
		if (lbtype->shapes[rotate].is_solid)
		{
			if (bshape->has[HILIGHT1]) memcpy(&colours[HILIGHT1], &colours[COLOUR1], CHANNELS);
			if (bshape->has[HILIGHT2]) memcpy(&colours[HILIGHT2], &colours[COLOUR2], CHANNELS);
		}
		if (rbtype->shapes[rotate].is_solid)
		{
			if (bshape->has[SHADOW1]) memcpy(&colours[SHADOW1], &colours[COLOUR1], CHANNELS);
			if (bshape->has[SHADOW2]) memcpy(&colours[SHADOW2], &colours[COLOUR2], CHANNELS);
		}

		// darken colours according to sky light (day) or block light (night)
		unsigned char nlight[4];
		if (chunk.slight != NULL)
		{
			get_neighbour_values(chunk.slight, chunk.nslight, nlight, rbx, y, rbz, rotate, 255);
			set_light_levels(colours, bshape, offset, chunk.slight, nlight,
					draw_top, draw_left, draw_right, 255);
		}
		else if (chunk.blight != NULL)
		{
			get_neighbour_values(chunk.blight, chunk.nblight, nlight, rbx, y, rbz, rotate, 0);
			set_light_levels(colours, bshape, offset, chunk.blight, nlight,
					draw_top, draw_left, draw_right, 0);
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
				unsigned char pcolour = bshape->pixels[sy * ISO_BLOCK_WIDTH + sx];
				if (pcolour == BLANK) continue;
				{
					int po = (py + sy) * img->width + px + sx;

					if ((sy < ISO_BLOCK_TOP_HEIGHT) ||
							(sx < ISO_BLOCK_WIDTH / 2 && draw_left) ||
							(sx >= ISO_BLOCK_WIDTH / 2 && draw_right))
						combine_alpha(colours[pcolour], &img->data[po * CHANNELS], 1);
				}
			}
		}
	}
}


static void get_block_alpha_colour(unsigned char *pixel, unsigned char *blocks,
		unsigned char *data, const textures *tex, const unsigned int offset)
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


static void render_ortho_block(image *img, const int cpx, const int cpy, const textures *tex,
		chunkdata chunk, const unsigned int rbx, const unsigned int rbz, const char rotate)
{
	// get pixel buffer for this block's rotated position
	unsigned char *pixel = &img->data[((cpy + rbz) * img->width + cpx + rbx) * CHANNELS];

	// get unrotated 2d block offset from rotated coordinates
	unsigned int hoffset = get_offset(0, rbx, rbz, rotate);

	for (int y = MAX_HEIGHT; y >= 0; y--)
	{
		// get unrotated 3d block offset
		unsigned int offset = y * CHUNK_BLOCK_AREA + hoffset;

		// skip air blocks or invalid block ids
		if (chunk.bids[offset] == 0 || chunk.bids[offset] >= tex->max_blockid) continue;

		// get block colour
		get_block_alpha_colour(pixel, chunk.bids, chunk.bdata, tex, offset);
		add_height_shading(pixel, y);

		// contour highlights and shadows
		unsigned char nbids[4];
		get_neighbour_values(chunk.bids, chunk.nbids, nbids, rbx, y, rbz, rotate, 0);
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


void render_chunk_map(image *img, const int cpx, const int cpy, nbt_node *chunk_nbt,
		nbt_node *nchunks_nbt[4], const unsigned int *cblimits, const textures *tex,
		const options *opts)
{
	// get block data for this chunk
	chunkdata chunk;
	chunk.bids = get_chunk_data(chunk_nbt, "Blocks", 0, 0, cblimits, opts->ylimits);
	chunk.bdata = get_chunk_data(chunk_nbt, "Data", 1, 0, cblimits, opts->ylimits);
	chunk.blight = opts->night ?
			get_chunk_data(chunk_nbt, "BlockLight", 1, 0, cblimits, opts->ylimits) : NULL;
	chunk.slight = (opts->isometric && !opts->night && opts->shadows) ?
			get_chunk_data(chunk_nbt, "SkyLight", 1, 255, cblimits, opts->ylimits) : NULL;

	// get block data for neighbouring chunks
	for (int i = 0; i < 4; i++)
	{
		if (nchunks_nbt[i] == NULL)
		{
			chunk.nbids[i] = NULL;
			chunk.nbdata[i] = NULL;
			chunk.nblight[i] = NULL;
			chunk.nslight[i] = NULL;
			continue;
		}
		chunk.nbids[i] = get_chunk_data(nchunks_nbt[i], "Blocks", 0, 0, cblimits, opts->ylimits);
		chunk.nbdata[i] = opts->isometric ?
				get_chunk_data(nchunks_nbt[i], "Data", 1, 0, cblimits, opts->ylimits) : NULL;
		chunk.nblight[i] = (opts->isometric && opts->night) ?
				get_chunk_data(nchunks_nbt[i], "BlockLight", 1, 0, cblimits, opts->ylimits) : NULL;
		chunk.nslight[i] = (opts->isometric && !opts->night && opts->shadows) ?
				get_chunk_data(nchunks_nbt[i], "SkyLight", 1, 255, cblimits, opts->ylimits) : NULL;
	}

	// loop through rotated chunk's blocks
	for (unsigned int rbz = 0; rbz <= MAX_CHUNK_BLOCK; rbz++)
		for (unsigned int rbx = 0; rbx <= MAX_CHUNK_BLOCK; rbx++)
			if (opts->isometric)
				render_iso_column(img, cpx, cpy, tex, chunk, rbx, rbz, opts->rotate);
			else
				render_ortho_block(img, cpx, cpy, tex, chunk, rbx, rbz, opts->rotate);

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
