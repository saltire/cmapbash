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


#include "chunk.h"
#include "image.h"
#include "textures.h"


#define WORLDDIR_PATH_MAXLEN 255
#define REGIONDIR_PATH_MAXLEN (WORLDDIR_PATH_MAXLEN + 7)
#define REGION_COORD_MAXLEN 8
#define REGIONFILE_PATH_MAXLEN (REGIONDIR_PATH_MAXLEN + MAX_REGION_COORD_LENGTH * 2 + 8)


typedef struct world
{
	char regiondir[REGIONDIR_PATH_MAXLEN];
	int rcount, rxmin, rxmax, rzmin, rzmax, rxsize, rzsize;
	unsigned char *regionmap;
}
world;


void render_tiny_world_map(image *image, int wpx, int wpy, world world, const options opts);
void render_world_map(image *image, int wpx, int wpy, world world, const textures *tex,
		const options opts);

void save_tiny_world_map(char *worlddir, const char *imagefile, const options opts);
void save_world_map(char *worlddir, const char *imagefile, const options opts);
