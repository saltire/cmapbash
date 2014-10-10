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


#ifndef MAP_H
#define MAP_H


#include "data.h"
#include "image.h"
#include "textures.h"


// configurable render options

#define HILIGHT_AMOUNT 0.125 // amount to lighten isometric block highlights
#define SHADOW_AMOUNT -0.125 // amount to darken isometric block shadows
#define HSHADE_HEIGHT 0.3 // height below which to add shadows
#define HSHADE_AMOUNT 0.7 // amount of shadow to add
#define NIGHT_AMBIENCE 0.2 // base light level for night renders

#define HSHADE_BLOCK_HEIGHT (HSHADE_HEIGHT * MAX_HEIGHT)


// data constants

#define LIGHT_LEVELS 16
#define MAX_LIGHT (LIGHT_LEVELS - 1)


// min/max macros

#define MIN(a,b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a < _b ? _a : _b; })

#define MAX(a,b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _a : _b; })


typedef struct options
{
	int isometric, night, shadows, tiny, use_limits;
	unsigned char rotate;
	int *limits, *ylimits;
	char *texpath, *shapepath;
}
options;


void render_iso_column(image *img, const int cpx, const int cpy, const textures *tex,
		chunk_data *chunk, const unsigned int rbx, const unsigned int rbz, const char rotate);

void render_ortho_block(image *img, const int cpx, const int cpy, const textures *tex,
		chunk_data *chunk, const unsigned int rbx, const unsigned int rbz, const char rotate);

void get_region_margins(unsigned int *margins, region *reg, const char rotate,
		const char isometric);

void render_tiny_region_map(image *img, const int rpx, const int rpy, region *reg,
		const options *opts);

void render_region_map(image *img, const int rpx, const int rpy, region *reg,
		region *nregions[4], const textures *tex, const options *opts);

void render_world_map(image *image, int wpx, int wpy, const worldinfo *world, const options *opts);

void save_world_map(char *worlddir, const char *imagefile, const options *opts);


#endif
