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


#ifndef TEXTURES_H
#define TEXTURES_H


#include "data.h"
#include "image.h"


// pixel dimensions for isometric rendering

#define ISO_BLOCK_WIDTH 4
#define ISO_BLOCK_TOP_HEIGHT 1
#define ISO_BLOCK_DEPTH 3
#define ISO_BLOCK_HEIGHT (ISO_BLOCK_TOP_HEIGHT + ISO_BLOCK_DEPTH)
#define ISO_BLOCK_AREA (ISO_BLOCK_WIDTH * ISO_BLOCK_HEIGHT)
#define ISO_BLOCK_X_MARGIN (ISO_BLOCK_WIDTH / 2)
#define ISO_BLOCK_Y_MARGIN ISO_BLOCK_TOP_HEIGHT

#define ISO_CHUNK_WIDTH (CHUNK_BLOCK_LENGTH * ISO_BLOCK_WIDTH)
#define ISO_CHUNK_TOP_HEIGHT ((CHUNK_BLOCK_LENGTH * 2 - 1) * ISO_BLOCK_TOP_HEIGHT)
#define ISO_CHUNK_DEPTH (ISO_BLOCK_DEPTH * CHUNK_BLOCK_HEIGHT)
#define ISO_CHUNK_HEIGHT (ISO_CHUNK_TOP_HEIGHT + ISO_CHUNK_DEPTH)
#define ISO_CHUNK_X_MARGIN (ISO_CHUNK_WIDTH / 2)
#define ISO_CHUNK_Y_MARGIN (CHUNK_BLOCK_LENGTH * ISO_BLOCK_TOP_HEIGHT)

#define ISO_REGION_WIDTH (ISO_CHUNK_WIDTH * REGION_CHUNK_LENGTH)
#define ISO_REGION_TOP_HEIGHT ((REGION_BLOCK_LENGTH * 2 - 1) * ISO_BLOCK_TOP_HEIGHT)
#define ISO_REGION_HEIGHT (ISO_REGION_TOP_HEIGHT + ISO_CHUNK_DEPTH)
#define ISO_REGION_X_MARGIN (ISO_REGION_WIDTH / 2)
#define ISO_REGION_Y_MARGIN (REGION_BLOCK_LENGTH * ISO_BLOCK_TOP_HEIGHT)


typedef enum
{
	BLANK,
	COLOUR1,
	HILIGHT1,
	SHADOW1,
	COLOUR2,
	HILIGHT2,
	SHADOW2,
	COLOUR_COUNT
}
colourcodes;

typedef struct shape
{
	char is_solid;
	char has[COLOUR_COUNT];
	unsigned char pixels[ISO_BLOCK_AREA];
}
shape;

typedef struct biome
{
	unsigned char foliage[CHANNELS];
	unsigned char grass[CHANNELS];
}
biome;

typedef struct blocktype
{
	unsigned char id;
	unsigned char subtype;
	unsigned char colours[COLOUR_COUNT][CHANNELS];
	char is_opaque;
	char biome_colour;
	shape shapes[4];
}
blocktype;

typedef struct blockID
{
	unsigned char mask;
	blocktype subtypes[16];
}
blockID;

typedef struct textures
{
	int max_blockid;
	blockID *blockids;
	biome *biomes;
}
textures;


textures *read_textures(const char *texpath, const char *shapepath, const char *biomepath);

void free_textures(textures *tex);

const blocktype *get_block_type(const textures *tex,
		const unsigned char blockid, const unsigned char dataval);

void set_colour_brightness(unsigned char *pixel, float brightness, float ambience);

void adjust_colour_brightness(unsigned char *pixel, float mod);

void combine_alpha(unsigned char *top, unsigned char *bottom, int down);


#endif
