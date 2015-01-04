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


// adjust a colour's brightness based on ambient light + light from another source
static void set_light_level(uint8_t *pixel, float brightness, float ambience)
{
	if (pixel[ALPHA] == 0) return;

	for (uint8_t c = 0; c < ALPHA; c++)
		// darken pixel to ambient light only, then add brightness
		pixel[c] *= (ambience + (1 - ambience) * brightness);
}


// in isometric night mode, adjust each face of the block to reflect sky/block light
static void set_block_light_levels(palette *palette, const shape *bshape, const uint16_t offset,
		const uint8_t *clight, const uint8_t nlight[], const uint8_t defval)
{
	int toffset = offset + CHUNK_BLOCK_AREA;
	float tlight = toffset > CHUNK_BLOCK_VOLUME ? defval : (float)clight[toffset];
	if (tlight < MAX_LIGHT)
	{
		tlight /= MAX_LIGHT;
		if (bshape->clrcount[COLOUR1])
			set_light_level((*palette)[COLOUR1], tlight, NIGHT_AMBIENCE);
		if (bshape->clrcount[COLOUR2])
			set_light_level((*palette)[COLOUR2], tlight, NIGHT_AMBIENCE);
	}
	if (nlight[BOTTOM_LEFT] < MAX_LIGHT)
	{
		float llight = (float)nlight[BOTTOM_LEFT] / MAX_LIGHT;
		if (bshape->clrcount[HILIGHT1])
			set_light_level((*palette)[HILIGHT1], llight, NIGHT_AMBIENCE);
		if (bshape->clrcount[HILIGHT2])
			set_light_level((*palette)[HILIGHT2], llight, NIGHT_AMBIENCE);
	}
	if (nlight[BOTTOM_RIGHT] < MAX_LIGHT)
	{
		float rlight = (float)nlight[BOTTOM_RIGHT] / MAX_LIGHT;
		if (bshape->clrcount[SHADOW1])
			set_light_level((*palette)[SHADOW1], rlight, NIGHT_AMBIENCE);
		if (bshape->clrcount[SHADOW2])
			set_light_level((*palette)[SHADOW2], rlight, NIGHT_AMBIENCE);
	}

}


void render_iso_column(image *img, const int32_t px, const int32_t py, const textures *tex,
		chunk_data *chunk, const uint8_t rbx, const uint8_t rbz, const options *opts)
{
	// get unrotated chunk-level 2d block offset from rotated coordinates
	uint8_t hoffset = get_block_offset(rbx, rbz, 0, opts->rotate);

	uint8_t biomeid = opts->biomes ? chunk->biomes[hoffset] : 0;

	uint8_t y = MAX_HEIGHT;
	do
	{
		// get unrotated chunk-level 3d block offset
		uint16_t offset = y * CHUNK_BLOCK_AREA + hoffset;

		uint8_t bid = chunk->bids[offset];
		uint8_t bdata = chunk->bdata[offset];

		// skip air blocks or invalid block ids
		if (bid == 0 || bid > tex->max_blockid) continue;

		// get block's pixel y coord
		uint32_t bpy = py + (MAX_HEIGHT - y) * ISO_BLOCK_DEPTH;

		// find which pixels are still unobscured, and skip this block if none are
		bool visible = 0;
		uint8_t mask[ISO_BLOCK_AREA];
		for (uint8_t sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
			for (uint8_t sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
			{
				uint8_t so = sy * ISO_BLOCK_WIDTH + sx;
				uint8_t alpha = img->data[((bpy + sy) * img->width + px + sx) * CHANNELS + ALPHA];
				mask[so] = alpha;
				if (alpha < 255) visible = 1;
			}
		if (!visible) continue;

		// get neighbour block ids and data values
		uint8_t nbids[4], nbdata[4];
		get_neighbour_values(nbids, chunk->bids, chunk->nbids, 0, rbx, rbz, y, opts->rotate);
		get_neighbour_values(nbdata, chunk->bdata, chunk->nbdata, 0, rbx, rbz, y, opts->rotate);

		// get the type of this block and overlapping blocks
		const blocktype *btype = get_block_type(tex, bid, bdata);
		const blocktype *tbtype = y == MAX_HEIGHT ? NULL : get_block_type(tex,
				chunk->bids[offset + CHUNK_BLOCK_AREA], chunk->bdata[offset + CHUNK_BLOCK_AREA]);
		const blocktype *lbtype = get_block_type(tex, nbids[BOTTOM_LEFT], nbdata[BOTTOM_LEFT]);
		const blocktype *rbtype = get_block_type(tex, nbids[BOTTOM_RIGHT], nbdata[BOTTOM_RIGHT]);

		// get block shape for this rotation
		const shape *bshape = &btype->shapes[opts->rotate];

		// get block colour palette, using biome colours if applicable
		palette palette;
		if (opts->biomes && btype->biome_palettes != NULL)
			memcpy(&palette, &btype->biome_palettes[biomeid], sizeof(palette));
		else
			memcpy(&palette, &btype->palette, sizeof(palette));

		// adjust colours for height
		for (uint8_t c = 0; c < COLOUR_COUNT; c++)
			if (bshape->clrcount[c]) add_height_shading(palette[c], y);

		// replace highlight and/or shadow with unshaded colour if that side is blocked
		if (lbtype->shapes[opts->rotate].clrcount[BLANK] == 0)
		{
			if (bshape->clrcount[HILIGHT1]) memcpy(&palette[HILIGHT1], &palette[COLOUR1], CHANNELS);
			if (bshape->clrcount[HILIGHT2]) memcpy(&palette[HILIGHT2], &palette[COLOUR2], CHANNELS);
		}
		if (rbtype->shapes[opts->rotate].clrcount[BLANK] == 0)
		{
			if (bshape->clrcount[SHADOW1]) memcpy(&palette[SHADOW1], &palette[COLOUR1], CHANNELS);
			if (bshape->clrcount[SHADOW2]) memcpy(&palette[SHADOW2], &palette[COLOUR2], CHANNELS);
		}

		// darken colours according to sky light (day + shadows) or block light (night)
		uint8_t nlight[4];
		if (chunk->slight != NULL)
		{
			get_neighbour_values(nlight, chunk->slight, chunk->nslight, 255,
					rbx, rbz, y, opts->rotate);
			set_block_light_levels(&palette, bshape, offset, chunk->slight, nlight, 255);
		}
		else if (chunk->blight != NULL)
		{
			get_neighbour_values(nlight, chunk->blight, chunk->nblight, 0,
					rbx, rbz, y, opts->rotate);
			set_block_light_levels(&palette, bshape, offset, chunk->blight, nlight, 0);
		}

		// draw pixels
		for (uint8_t sy = 0; sy < ISO_BLOCK_HEIGHT; sy++)
			for (uint8_t sx = 0; sx < ISO_BLOCK_WIDTH; sx++)
			{
				uint8_t so = sy * ISO_BLOCK_WIDTH + sx;
				uint8_t pcolour = bshape->pixmap[so];
				if (pcolour == BLANK || mask[so] == 255) continue;
				{
					uint32_t po = (bpy + sy) * img->width + px + sx;
					combine_alpha(&img->data[po * CHANNELS], palette[pcolour], 0);
				}
			}
	}
	while (y-- > 0);
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


void render_ortho_column(image *img, const int32_t px, const int32_t py, const textures *tex,
		chunk_data *chunk, const uint32_t rbx, const uint32_t rbz, const options *opts)
{
	// get pixel buffer for this block's rotated position
	uint8_t *pixel = &img->data[(py * img->width + px) * CHANNELS];

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
		bool light = (nbids[TOP] == 0 || nbids[LEFT] == 0);
		bool dark = (nbids[BOTTOM] == 0 || nbids[RIGHT] == 0);
		if (light && !dark) adjust_colour_brightness(pixel, HILIGHT_AMOUNT);
		if (dark && !light) adjust_colour_brightness(pixel, SHADOW_AMOUNT);

		// night mode: darken colours according to block light
		if (opts->night) {
			float tbl = y < MAX_HEIGHT ? chunk->blight[offset + CHUNK_BLOCK_AREA] : 0;
			if (tbl < MAX_LIGHT) set_light_level(pixel, tbl / MAX_LIGHT, NIGHT_AMBIENCE);
		}

		break;
	}
}
