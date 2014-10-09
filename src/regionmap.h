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


#ifndef REGIONMAP_H
#define REGIONMAP_H


#include "image.h"
#include "regiondata.h"
#include "textures.h"


typedef struct options
{
	int isometric, night, shadows, tiny, use_limits;
	unsigned char rotate;
	int *limits, *ylimits;
	char *texpath, *shapepath;
}
options;


void get_region_margins(unsigned int *margins, region *reg, const char rotate,
		const char isometric);

void render_tiny_region_map(image *img, const int rpx, const int rpy, region *reg,
		const options *opts);
void render_region_map(image *img, const int rpx, const int rpy, region *reg,
		region *nregions[4], const textures *tex, const options *opts);


#endif
