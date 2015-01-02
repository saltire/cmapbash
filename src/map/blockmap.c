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


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "image.h"
#include "map.h"
#include "textures.h"


// add a shadow to blocks below a certain height
static void add_height_shading(unsigned char *pixel, const unsigned int y)
{
	if (pixel[ALPHA] == 0 || y >= HSHADE_BLOCK_HEIGHT) return;
	adjust_colour_brightness(pixel, (((float)y / HSHADE_BLOCK_HEIGHT) - 1) * HSHADE_AMOUNT);
}


// in isometric night mode, adjust each face of the block to reflect sky/block light
static void set_light_levels(palette *palette, const shape *bshape, const uint16_t offset,
		const uint8_t *clight, const uint8_t nlight[], const uint8_t defval,
		const bool draw_t, const bool draw_l, const bool draw_r)
{
	int toffset = offset + CHUNK_BLOCK_AREA;
	float tlight = toffset > CHUNK_BLOCK_VOLUME ? defval : (float)clight[toffset];
	if (draw_t && tlight < MAX_LIGHT)
	{
		tlight /= MAX_LIGHT;
		set_colour_brightness((*palette)[COLOUR1], tlight, NIGHT_AMBIENCE);
		if (HAS_PTR(bshape, COLOUR2))
			set_colour_brightness((*palette)[COLOUR2], tlight, NIGHT_AMBIENCE);
	}
	if (draw_l && nlight[2] < MAX_LIGHT)
	{
		float llight = (float)nlight[2] / MAX_LIGHT;
		if (HAS_PTR(bshape, HILIGHT1))
			set_colour_brightness((*palette)[HILIGHT1], llight, NIGHT_AMBIENCE);
		if (HAS_PTR(bshape, HILIGHT2))
			set_colour_brightness((*palette)[HILIGHT2], llight, NIGHT_AMBIENCE);
	}
	if (draw_r && nlight[1] < MAX_LIGHT)
	{
		float rlight = (float)nlight[1] / MAX_LIGHT;
		if (HAS_PTR(bshape, SHADOW1))
			set_colour_brightness((*palette)[SHADOW1], rlight, NIGHT_AMBIENCE);
		if (HAS_PTR(bshape, SHADOW2))
			set_colour_brightness((*palette)[SHADOW2], rlight, NIGHT_AMBIENCE);
	}

}


void render_iso_column(image *img, const int32_t cpx, const int32_t cpy, const textures *tex,
		chunk_data *chunk, const uint8_t rbx, const uint8_t rbz, const options *opts)
{
	// get unrotated chunk-level 2d block offset from rotated coordinates
	uint8_t hoffset = get_block_offset(rbx, rbz, 0, opts->rotate);

	uint8_t biomeid = chunk->biomes[hoffset];

	for (uint8_t y = 0; y <= MAX_HEIGHT; y++)
	{
		// get unrotated chunk-level 3d block offset
		uint16_t offset = y * CHUNK_BLOCK_AREA + hoffset;

		uint8_t bid = chunk->bids[offset];
		uint8_t bdata = chunk->bdata[offset];

		// skip air blocks or invalid block ids
		if (bid == 0 || bid > tex->max_blockid) continue;

		// get neighbour block ids and data values
		uint8_t nbids[4], nbdata[4];
		get_neighbour_values(nbids, chunk->bids, chunk->nbids, 0, rbx, rbz, y, opts->rotate);
		get_neighbour_values(nbdata, chunk->bdata, chunk->nbdata, 0, rbx, rbz, y, opts->rotate);

		// get the type of this block and neighbouring blocks
		const blocktype *btype = get_block_type(tex, bid, bdata);
		const blocktype *tbtype = y == MAX_HEIGHT ? NULL : get_block_type(tex,
				chunk->bids[offset + CHUNK_BLOCK_AREA], chunk->bdata[offset + CHUNK_BLOCK_AREA]);
		const blocktype *lbtype = get_block_type(tex, nbids[2], nbdata[2]);
		const blocktype *rbtype = get_block_type(tex, nbids[1], nbdata[1]);

		// if the block above is the same type or opaque, don't draw the top of this one
		bool draw_top = !(y < MAX_HEIGHT && (
				(tbtype->id == btype->id && tbtype->subtype == btype->subtype) ||
				(tbtype->shapes[opts->rotate].is_solid && tbtype->is_opaque)));

		// if block in front of left or right side is solid and opaque, don't draw that side
		bool draw_left = !(lbtype->shapes[opts->rotate].is_solid && lbtype->is_opaque);
		bool draw_right = !(rbtype->shapes[opts->rotate].is_solid && rbtype->is_opaque);

		// skip this block if we aren't drawing any of the faces
		if (!draw_top && !draw_left && !draw_right) continue;

		// get block shape for this rotation
		const shape *bshape = &btype->shapes[opts->rotate];

		// get block colour palette, using biome colours if applicable
		palette palette;
		if (opts->biomes && btype->biome_palettes != NULL)
			memcpy(&palette, &btype->biome_palettes[biomeid], sizeof(palette));
		else
			memcpy(&palette, &btype->palette, sizeof(palette));

		// adjust for height
		for (uint8_t i = 0; i < COLOUR_COUNT; i++)
			if (HAS_PTR(bshape, i)) add_height_shading(palette[i], y);

		// replace highlight and/or shadow with unshaded colour if that side is blocked
		// FIXME: this is inexact; hilights and shadows don't necessarily appear only on one side
		if (lbtype->shapes[opts->rotate].is_solid)
		{
			if (HAS_PTR(bshape, HILIGHT1)) memcpy(&palette[HILIGHT1], &palette[COLOUR1], CHANNELS);
			if (HAS_PTR(bshape, HILIGHT2)) memcpy(&palette[HILIGHT2], &palette[COLOUR2], CHANNELS);
		}
		if (rbtype->shapes[opts->rotate].is_solid)
		{
			if (HAS_PTR(bshape, SHADOW1)) memcpy(&palette[SHADOW1], &palette[COLOUR1], CHANNELS);
			if (HAS_PTR(bshape, SHADOW2)) memcpy(&palette[SHADOW2], &palette[COLOUR2], CHANNELS);
		}

		// darken colours according to sky light (day) or block light (night)
		uint8_t nlight[4];
		if (chunk->slight != NULL)
		{
			get_neighbour_values(nlight, chunk->slight, chunk->nslight, 255,
					rbx, rbz, y, opts->rotate);
			set_light_levels(&palette, bshape, offset, chunk->slight, nlight, 255,
					draw_top, draw_left, draw_right);
		}
		else if (chunk->blight != NULL)
		{
			get_neighbour_values(nlight, chunk->blight, chunk->nblight, 0,
					rbx, rbz, y, opts->rotate);
			set_light_levels(&palette, bshape, offset, chunk->blight, nlight, 0,
					draw_top, draw_left, draw_right);
		}

		// translate orthographic to isometric coordinates
		uint32_t px = cpx + (rbx + MAX_CHUNK_BLOCK - rbz) * ISO_BLOCK_WIDTH / 2;
		uint32_t py = cpy + (rbx + rbz) * ISO_BLOCK_TOP_HEIGHT + (MAX_HEIGHT - y) * ISO_BLOCK_DEPTH;

		// draw pixels
		for (uint8_t sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
		{
			// skip top rows if we aren't drawing them
			if (sy < ISO_BLOCK_TOP_HEIGHT && !draw_top) continue;
			for (uint8_t sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
			{
				uint8_t pcolour = bshape->pixels[sy * ISO_BLOCK_WIDTH + sx];
				if (pcolour == BLANK) continue;
				{
					uint32_t po = (py + sy) * img->width + px + sx;

					if ((sy < ISO_BLOCK_TOP_HEIGHT) ||
							// skip left/right columns if we aren't drawing them
							(sx < ISO_BLOCK_WIDTH / 2 && draw_left) ||
							(sx >= ISO_BLOCK_WIDTH / 2 && draw_right))
						combine_alpha(palette[pcolour], &img->data[po * CHANNELS], 1);
				}
			}
		}
	}
}


// get the colour of this block, alpha blended with any visible blocks below it
static void get_block_alpha_colour(uint8_t *pixel, chunk_data *chunk, const textures *tex,
		const uint16_t offset, const bool use_biome, const uint8_t biomeid)
{
	const blocktype *btype = get_block_type(tex, chunk->bids[offset], chunk->bdata[offset]);

	// copy the block colour into the pixel buffer, using biome colour if applicable
	if (use_biome && btype->biome_palettes != NULL)
		memcpy(pixel, btype->biome_palettes[biomeid][COLOUR1], CHANNELS);
	else
		memcpy(pixel, btype->palette[COLOUR1], CHANNELS);

	// if block colour is not fully opaque, combine with the block below it
	if (pixel[ALPHA] < 255)
	{
		// get the next block down, or use black if this is the bottom block
		uint8_t next[CHANNELS] = {0};
		if (offset > CHUNK_BLOCK_AREA)
			get_block_alpha_colour(next, chunk, tex, offset - CHUNK_BLOCK_AREA, use_biome, biomeid);
		combine_alpha(pixel, next, 0);
	}
}


void render_ortho_column(image *img, const int32_t cpx, const int32_t cpy, const textures *tex,
		chunk_data *chunk, const uint32_t rbx, const uint32_t rbz, const options *opts)
{
	// get pixel buffer for this block's rotated position
	uint8_t *pixel = &img->data[((cpy + rbz) * img->width + cpx + rbx) * CHANNELS];

	// get unrotated 2d block offset from rotated coordinates
	uint16_t hoffset = get_block_offset(rbx, rbz, 0, opts->rotate);

	uint8_t biomeid = opts->biomes ? chunk->biomes[hoffset] : 0;

	for (uint8_t y = MAX_HEIGHT; y >= 0; y--)
	{
		// get unrotated 3d block offset
		uint16_t offset = y * CHUNK_BLOCK_AREA + hoffset;

		// skip air blocks or invalid block ids
		if (chunk->bids[offset] == 0 || chunk->bids[offset] >= tex->max_blockid) continue;

		// get block colour
		get_block_alpha_colour(pixel, chunk, tex, offset, opts->biomes, biomeid);
		add_height_shading(pixel, y);

		// contour highlights and shadows
		uint8_t nbids[4];
		get_neighbour_values(nbids, chunk->bids, chunk->nbids, 0, rbx, rbz, y, opts->rotate);
		bool light = (nbids[0] == 0 || nbids[3] == 0);
		bool dark = (nbids[1] == 0 || nbids[2] == 0);
		if (light && !dark) adjust_colour_brightness(pixel, HILIGHT_AMOUNT);
		if (dark && !light) adjust_colour_brightness(pixel, SHADOW_AMOUNT);

		// night mode: darken colours according to block light
		if (opts->night) {
			float tbl = y < MAX_HEIGHT ? chunk->blight[offset + CHUNK_BLOCK_AREA] : 0;
			if (tbl < MAX_LIGHT) set_colour_brightness(pixel, tbl / MAX_LIGHT, NIGHT_AMBIENCE);
		}

		break;
	}
}
