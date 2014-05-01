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


#include "dims.h"
#include "image.h"


// configurable render options
#define HILIGHT_AMOUNT 0.125
#define SHADOW_AMOUNT -0.125

typedef enum {
	BLANK,
	COLOUR1,
	HILIGHT1,
	SHADOW1,
	COLOUR2,
	HILIGHT2,
	SHADOW2,
	COLOUR_COUNT
} colourcodes;

typedef struct shape {
	char is_solid;
	char has[COLOUR_COUNT];
	unsigned char pixels[ISO_BLOCK_AREA];
} shape;

typedef struct blocktype {
	unsigned char id;
	unsigned char subtype;
	unsigned char colours[COLOUR_COUNT][CHANNELS];
	char is_opaque;
	shape shapes[4];
} blocktype;

typedef struct blockID {
	unsigned char mask;
	blocktype subtypes[16];
} blockID;

typedef struct textures {
	int max_blockid;
	blockID* blockids;
} textures;


textures* read_textures(const char* texturefile, const char* shapefile);
void free_textures(textures* tex);

const blocktype* get_block_type(const textures* tex,
		const unsigned char blockid, const unsigned char dataval);

void set_colour_brightness(unsigned char* pixel, float brightness, float ambience);
void adjust_colour_brightness(unsigned char* pixel, float mod);
void combine_alpha(unsigned char* top, unsigned char* bottom, int down);


#endif
