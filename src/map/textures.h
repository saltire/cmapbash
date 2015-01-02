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


#include <stdbool.h>
#include <stdint.h>

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


#define BLOCK_SUBTYPES 16

// check whether a shape has any pixels of a colour
#define HAS(shape, colour) ((shape).has & (1 << colour))
#define HAS_PTR(shape, colour) ((shape)->has & (1 << colour))


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
colours;

typedef struct shape
{
	bool is_solid;
	uint8_t has; // a bitfield with a bit for each colour in the palette
	uint8_t pixels[ISO_BLOCK_AREA];
}
shape;

typedef struct biome
{
	bool exists;
	uint8_t foliage[CHANNELS];
	uint8_t grass[CHANNELS];
}
biome;

typedef unsigned char palette[COLOUR_COUNT][CHANNELS];

typedef struct blocktype
{
	uint8_t id;
	uint8_t subtype;
	palette palette;
	palette *biome_palettes;
	bool is_opaque;
	shape shapes[4];
}
blocktype;

typedef struct blockID
{
	uint8_t mask;
	blocktype subtypes[BLOCK_SUBTYPES];
}
blockID;

typedef struct textures
{
	uint8_t max_blockid;
	blockID *blockids;
}
textures;


/* generate a texture struct from a set of CSV files
 *   texpath:   path to the block texture/colour CSV
 *   shapepath: path to the isometric shape CSV (or NULL if not rendering an isometric map)
 *   biomepath: path to the biome colour CSV (or NULL if not rendering biomes)
 */
textures *read_textures(const char *texpath, const char *shapepath, const char *biomepath);

/* free the memory used for a texture struct
 *   tex: pointer to the texture struct
 */
void free_textures(textures *tex);

/* get a block type struct given a block ID and a data value
 *   tex:     pointer to the texture struct
 *   blockid: the ID of the block's type
 *   dataval: the data value of the block, if any
 */
const blocktype *get_block_type(const textures *tex, const uint8_t blockid, const uint8_t dataval);

/* set an RGB colour to a certain brightness
 *   pixel:      pointer to the colour or pixel buffer
 *   brightness: a float value from 0 to 1 representing the amount of light above the minimum
 *   ambience:   a float value from 0 to 1 representing the minimum brightness
 */
void set_colour_brightness(uint8_t *pixel, float brightness, float ambience);

/* make an RGB colour lighter or darker
 *   pixel: pointer to the colour or pixel buffer
 *   mod:   a float value from -1 to 1 representing the percentage to lighten or darken
 */
void adjust_colour_brightness(unsigned char *pixel, float mod);

/* alpha blend one RGBA colour on top of another, replacing one of them
 *   top:    pointer to the top colour or pixel buffer
 *   bottom: pointer to the bottom colour or pixel buffer
 *   down:   whether to store the result in the bottom buffer (true) or the top buffer (false)
 */
void combine_alpha(uint8_t *top, uint8_t *bottom, bool down);


#endif
