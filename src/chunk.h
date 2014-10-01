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


#ifndef CHUNK_H
#define CHUNK_H


#include "nbt.h"

#include "image.h"
#include "textures.h"


typedef struct options
{
	int isometric, night, shadows, tiny, use_limits;

	unsigned char rotate;

	int *limits, *ylimits;

	char *texpath;
	char *shapepath;
}
options;


void render_chunk_map(image *image, const int cpx, const int cpy, nbt_node *chunk_nbt,
		nbt_node *nchunks_nbt[4], const unsigned int *cblimits, const textures *tex,
		const options *opts);

void save_chunk_map(nbt_node *chunk, const char *imagefile, const options *opts);


#endif
