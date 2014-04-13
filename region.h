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
#include "textures.h"


// world dimensions
#define REGION_CHUNK_LENGTH 32
#define REGION_BLOCK_LENGTH (REGION_CHUNK_LENGTH * CHUNK_BLOCK_LENGTH)
#define REGION_CHUNK_AREA (REGION_CHUNK_LENGTH * REGION_CHUNK_LENGTH)
#define REGION_BLOCK_AREA (REGION_CHUNK_AREA * CHUNK_BLOCK_AREA)

// isometric pixel dimensions
#define ISO_REGION_WIDTH (ISO_CHUNK_WIDTH * REGION_CHUNK_LENGTH)
#define ISO_REGION_TOP_HEIGHT ((REGION_BLOCK_LENGTH * 2 - 1) * ISO_BLOCK_TOP_HEIGHT)
#define ISO_REGION_HEIGHT (ISO_REGION_TOP_HEIGHT + ISO_CHUNK_DEPTH)
#define ISO_REGION_X_MARGIN (ISO_REGION_WIDTH / 2)
#define ISO_REGION_Y_MARGIN (REGION_BLOCK_LENGTH * ISO_BLOCK_TOP_HEIGHT)


void get_region_margins(const char* regionfile, int* margins, const char rotate);
void get_region_iso_margins(const char* regionfile, int* margins, const char rotate);

void render_region_map(image* image, const int rpx, const int rpy,
		const char* regionfile, char* nfiles[4], const textures* tex,
		const char night, const char isometric, const char rotate);
void save_region_map(const char* regionfile, const char* imagefile, const textures* tex,
		const char night, const char isometric, const char rotate);
