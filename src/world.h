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


#ifndef WORLD_H
#define WORLD_H


#include "chunk.h"
#include "dims.h"
#include "image.h"
#include "region.h"
#include "textures.h"


typedef struct world
{
	char regiondir[REGIONDIR_PATH_MAXLEN];
	int rcount, rrxsize, rrzsize, rrxmax, rrzmax;
	unsigned char rotate;
	region *regions;
	region **regionmap;
}
world;


void render_tiny_world_map(image *image, int wpx, int wpy, const world *world, const options *opts);
void render_world_map(image *image, int wpx, int wpy, const world *world, const textures *tex,
		const options *opts);

void save_tiny_world_map(char *worlddir, const char *imagefile, const options *opts);
void save_world_map(char *worlddir, const char *imagefile, const options *opts);


#endif
