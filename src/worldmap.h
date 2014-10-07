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


#ifndef WORLDMAP_H
#define WORLDMAP_H


#include "chunkmap.h"
#include "image.h"
#include "worlddata.h"


void render_world_map(image *image, int wpx, int wpy, const worldinfo *world, const options *opts);

void save_world_map(char *worlddir, const char *imagefile, const options *opts);


#endif
