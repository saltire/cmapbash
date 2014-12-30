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

#include "data.h"
#include "image.h"
#include "map.h"
#include "textures.h"


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


void render_iso_column(image *img, const int cpx, const int cpy, const textures *tex,
		chunk_data *chunk, const unsigned int rbx, const unsigned int rbz, const options *opts)
{
	// get unrotated 2d block offset from rotated coordinates
	unsigned int hoffset = get_block_offset(0, rbx, rbz, opts->rotate);

	biome *biome = &tex->biomes[chunk->biomes[hoffset]];

	for (unsigned int y = 0; y <= MAX_HEIGHT; y++)
	{
		// get unrotated 3d block offset
		unsigned int offset = y * CHUNK_BLOCK_AREA + hoffset;

		unsigned char bid = chunk->bids[offset];
		unsigned char bdata = chunk->bdata[offset];

		// skip air blocks or invalid block ids
		if (bid == 0 || bid > tex->max_blockid) continue;

		// get neighbour block ids and data values
		unsigned char nbids[4], nbdata[4];
		get_neighbour_values(nbids, chunk->bids, chunk->nbids, rbx, y, rbz, opts->rotate, 0);
		get_neighbour_values(nbdata, chunk->bdata, chunk->nbdata, rbx, y, rbz, opts->rotate, 0);

		// get the type of this block and neighbouring blocks
		const blocktype *btype = get_block_type(tex, bid, bdata);
		const blocktype *tbtype = y == MAX_HEIGHT ? NULL : get_block_type(tex,
				chunk->bids[offset + CHUNK_BLOCK_AREA], chunk->bdata[offset + CHUNK_BLOCK_AREA]);
		const blocktype *lbtype = get_block_type(tex, nbids[2], nbdata[2]);
		const blocktype *rbtype = get_block_type(tex, nbids[1], nbdata[1]);

		// if the block above is the same type or opaque, don't draw the top of this one
		char draw_top = !(y < MAX_HEIGHT && (
				(tbtype->id == btype->id && tbtype->subtype == btype->subtype) ||
				(tbtype->shapes[opts->rotate].is_solid && tbtype->is_opaque)));

		// if block in front of left or right side is solid and opaque, don't draw that side
		char draw_left = !(lbtype->shapes[opts->rotate].is_solid && lbtype->is_opaque);
		char draw_right = !(rbtype->shapes[opts->rotate].is_solid && rbtype->is_opaque);

		// skip this block if we aren't drawing any of the faces
		if (!draw_top && !draw_left && !draw_right) continue;

		// get block shape for this rotation
		const shape *bshape = &btype->shapes[opts->rotate];

		// get block colours
		unsigned char colours[COLOUR_COUNT][CHANNELS];
		memcpy(&colours, btype->colours, COLOUR_COUNT * CHANNELS);

		// use biome colours if applicable
		if (opts->biomes && btype->biome_colour1)
			memcpy(&colours[COLOUR1], &btype->biome_colours[COLOUR1], CHANNELS * 3);
		if (opts->biomes && btype->biome_colour2)
			memcpy(&colours[COLOUR2], &btype->biome_colours[COLOUR2], CHANNELS * 3);

		// adjust for height
		for (int i = 0; i < COLOUR_COUNT; i++)
			if (bshape->has[i]) add_height_shading(colours[i], y);

		// replace highlight and/or shadow with unshaded colour if that side is blocked
		if (lbtype->shapes[opts->rotate].is_solid)
		{
			if (bshape->has[HILIGHT1]) memcpy(&colours[HILIGHT1], &colours[COLOUR1], CHANNELS);
			if (bshape->has[HILIGHT2]) memcpy(&colours[HILIGHT2], &colours[COLOUR2], CHANNELS);
		}
		if (rbtype->shapes[opts->rotate].is_solid)
		{
			if (bshape->has[SHADOW1]) memcpy(&colours[SHADOW1], &colours[COLOUR1], CHANNELS);
			if (bshape->has[SHADOW2]) memcpy(&colours[SHADOW2], &colours[COLOUR2], CHANNELS);
		}

		// darken colours according to sky light (day) or block light (night)
		unsigned char nlight[4];
		if (chunk->slight != NULL)
		{
			get_neighbour_values(nlight, chunk->slight, chunk->nslight,
					rbx, y, rbz, opts->rotate, 255);
			set_light_levels(colours, bshape, offset, chunk->slight, nlight,
					draw_top, draw_left, draw_right, 255);
		}
		else if (chunk->blight != NULL)
		{
			get_neighbour_values(nlight, chunk->blight, chunk->nblight,
					rbx, y, rbz, opts->rotate, 0);
			set_light_levels(colours, bshape, offset, chunk->blight, nlight,
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


static void get_block_alpha_colour(unsigned char *pixel, chunk_data *chunk, const textures *tex,
		const unsigned int offset, unsigned char biome)
{
	const blocktype *btype = get_block_type(tex, chunk->bids[offset], chunk->bdata[offset]);

	// copy the block colour into the pixel buffer
	if (biome != NULL && btype->biome_colour1)
		memcpy(pixel, btype->biome_colours[biome][COLOUR1 * CHANNELS], CHANNELS);
	else
		memcpy(pixel, btype->colours[COLOUR1], CHANNELS);

	// if block colour is not fully opaque, combine with the block below it
	if (pixel[ALPHA] < 255)
	{
		// get the next block down, or use black if this is the bottom block
		unsigned char next[CHANNELS] = {0};
		if (offset > CHUNK_BLOCK_AREA)
			get_block_alpha_colour(next, chunk, tex, offset - CHUNK_BLOCK_AREA, biome);
		combine_alpha(pixel, next, 0);
	}
}


void render_ortho_block(image *img, const int cpx, const int cpy, const textures *tex,
		chunk_data *chunk, const unsigned int rbx, const unsigned int rbz, const options *opts)
{
	// get pixel buffer for this block's rotated position
	unsigned char *pixel = &img->data[((cpy + rbz) * img->width + cpx + rbx) * CHANNELS];

	// get unrotated 2d block offset from rotated coordinates
	unsigned int hoffset = get_block_offset(0, rbx, rbz, opts->rotate);

	unsigned char biome = opts->biomes ? chunk->biomes[hoffset] : NULL;

	for (int y = MAX_HEIGHT; y >= 0; y--)
	{
		// get unrotated 3d block offset
		unsigned int offset = y * CHUNK_BLOCK_AREA + hoffset;

		// skip air blocks or invalid block ids
		if (chunk->bids[offset] == 0 || chunk->bids[offset] >= tex->max_blockid) continue;

		// get block colour
		get_block_alpha_colour(pixel, chunk, tex, offset, biome);
		add_height_shading(pixel, y);

		// contour highlights and shadows
		unsigned char nbids[4];
		get_neighbour_values(nbids, chunk->bids, chunk->nbids, rbx, y, rbz, opts->rotate, 0);
		char light = (nbids[0] == 0 || nbids[3] == 0);
		char dark = (nbids[1] == 0 || nbids[2] == 0);
		if (light && !dark) adjust_colour_brightness(pixel, HILIGHT_AMOUNT);
		if (dark && !light) adjust_colour_brightness(pixel, SHADOW_AMOUNT);

		// night mode: darken colours according to block light
		if (chunk->blight != NULL) {
			float tbl = y < MAX_HEIGHT ? chunk->blight[offset + CHUNK_BLOCK_AREA] : 0;
			if (tbl < MAX_LIGHT) set_colour_brightness(pixel, tbl / MAX_LIGHT, NIGHT_AMBIENCE);
		}

		break;
	}
}
