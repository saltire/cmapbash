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


#ifndef DIMS_H
#define DIMS_H


// world dimensions

#define CHUNK_BLOCK_LENGTH 16
#define CHUNK_BLOCK_BITS 4
#define CHUNK_SECTION_HEIGHT 16
#define SECTION_BLOCK_HEIGHT 16
#define CHUNK_BLOCK_AREA (CHUNK_BLOCK_LENGTH * CHUNK_BLOCK_LENGTH)
#define CHUNK_BLOCK_HEIGHT (SECTION_BLOCK_HEIGHT * CHUNK_SECTION_HEIGHT)
#define SECTION_BLOCK_VOLUME (SECTION_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)
#define CHUNK_BLOCK_VOLUME (CHUNK_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)

#define REGION_CHUNK_LENGTH 32
#define REGION_CHUNK_BITS 5
#define REGION_BLOCK_LENGTH (REGION_CHUNK_LENGTH * CHUNK_BLOCK_LENGTH)
#define REGION_BLOCK_BITS (REGION_CHUNK_BITS + CHUNK_BLOCK_BITS)
#define REGION_CHUNK_AREA (REGION_CHUNK_LENGTH * REGION_CHUNK_LENGTH)
#define REGION_BLOCK_AREA (REGION_CHUNK_AREA * CHUNK_BLOCK_AREA)

#define LIGHT_LEVELS 16


// maximum values

#define MAX_CHUNK_BLOCK (CHUNK_BLOCK_LENGTH - 1)
#define MAX_REGION_CHUNK (REGION_CHUNK_LENGTH - 1)
#define MAX_REGION_BLOCK (REGION_BLOCK_LENGTH - 1)
#define MAX_HEIGHT (CHUNK_BLOCK_HEIGHT - 1)
#define MAX_LIGHT (LIGHT_LEVELS - 1)


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


// path lengths

#define WORLDDIR_PATH_MAXLEN 255
#define REGIONDIR_PATH_MAXLEN (WORLDDIR_PATH_MAXLEN + 7)
#define REGION_COORD_MAXLEN 8
#define REGIONFILE_PATH_MAXLEN (REGIONDIR_PATH_MAXLEN + REGION_COORD_MAXLEN * 2 + 8)


// min/max macros

#define MIN(a,b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a < _b ? _a : _b; })

#define MAX(a,b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _a : _b; })


unsigned int get_offset(const unsigned int y, const unsigned int rx, const unsigned int rz,
		const unsigned int length, const unsigned char rotate);


#endif
