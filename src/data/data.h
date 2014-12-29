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


#ifndef DATA_H_
#define DATA_H_


#include "nbt.h"


// world dimensions

#define CHUNK_BLOCK_BITS 4
#define REGION_CHUNK_BITS 5
#define REGION_BLOCK_BITS (REGION_CHUNK_BITS + CHUNK_BLOCK_BITS)

#define CHUNK_BLOCK_LENGTH (1 << CHUNK_BLOCK_BITS)
#define CHUNK_BLOCK_AREA (CHUNK_BLOCK_LENGTH * CHUNK_BLOCK_LENGTH)
#define SECTION_BLOCK_HEIGHT 16
#define CHUNK_SECTION_HEIGHT 16
#define CHUNK_BLOCK_HEIGHT (SECTION_BLOCK_HEIGHT * CHUNK_SECTION_HEIGHT)
#define SECTION_BLOCK_VOLUME (SECTION_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)
#define CHUNK_BLOCK_VOLUME (CHUNK_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)

#define REGION_CHUNK_LENGTH (1 << REGION_CHUNK_BITS)
#define REGION_BLOCK_LENGTH (1 << REGION_BLOCK_BITS)
#define REGION_CHUNK_AREA (REGION_CHUNK_LENGTH * REGION_CHUNK_LENGTH)
#define REGION_BLOCK_AREA (REGION_CHUNK_AREA * CHUNK_BLOCK_AREA)


// maximum values

#define MAX_CHUNK_BLOCK (CHUNK_BLOCK_LENGTH - 1)
#define MAX_REGION_CHUNK (REGION_CHUNK_LENGTH - 1)
#define MAX_REGION_BLOCK (REGION_BLOCK_LENGTH - 1)
#define MAX_HEIGHT (CHUNK_BLOCK_HEIGHT - 1)


// path lengths

#define WORLDDIR_PATH_MAXLEN 255
#define REGIONDIR_PATH_MAXLEN (WORLDDIR_PATH_MAXLEN + 7)
#define REGION_COORD_MAXLEN 8
#define REGIONFILE_PATH_MAXLEN (REGIONDIR_PATH_MAXLEN + REGION_COORD_MAXLEN * 2 + 8)


// region file constants

#define SECTOR_BYTES 4096
#define OFFSET_BYTES 4
#define LENGTH_BYTES 4


typedef struct chunk_data
{
	nbt_node *nbt;
	unsigned int *blimits;
	unsigned char *bids, *bdata, *blight, *slight, *biomes;
	unsigned char *nbids[4], *nbdata[4], *nblight[4], *nslight[4];
}
chunk_data;


typedef struct chunk_flags
{
	char bids, bdata, blight, slight, biomes;
}
chunk_flags;


typedef struct region
{
	int x, z;
	char path[REGIONFILE_PATH_MAXLEN];
	unsigned int offsets[REGION_CHUNK_AREA];
	unsigned int *blimits, *cblimits[REGION_CHUNK_AREA];
	FILE *file;
}
region;


typedef struct worldinfo
{
	char regiondir[REGIONDIR_PATH_MAXLEN];
	unsigned int rcount, rrxsize, rrzsize, rrxmax, rrzmax;
	unsigned char rotate;
	region **regionmap;
}
worldinfo;


unsigned int get_block_offset(const unsigned int y, const unsigned int rbx,
		const unsigned int rbz, const unsigned char rotate);

unsigned int get_chunk_offset(const unsigned int rcx, const unsigned int rcz,
		const char rotate);

void get_neighbour_values(unsigned char nvalues[4], unsigned char *data, unsigned char *ndata[4],
		const int rbx, const int y, const int rbz, const char rotate, unsigned char defval);

unsigned char *get_chunk_data(chunk_data *chunk, unsigned char *tag, const char half,
		const char defval, const unsigned int *ylimits);

chunk_data *parse_chunk_nbt(const char *cdata, const unsigned int length, const chunk_flags *flags,
		unsigned int *cblimits, const unsigned int *ylimits);

chunk_data *read_chunk(const region *reg, const unsigned int rcx, const unsigned int rcz,
		const char rotate, const chunk_flags *flags, const unsigned int *ylimits);

void free_chunk(chunk_data *chunk);

char chunk_exists(const region *reg, const unsigned int rcx, const unsigned int rcz,
		const unsigned char rotate);

FILE *open_region_file(region *reg);

void close_region_file(region *reg);

region *read_region(const char *regiondir, const int rx, const int rz,
		const unsigned int *rblimits);

void free_region(region *reg);

region *get_region_from_coords(const worldinfo *world, const int rrx, const int rrz);

worldinfo *measure_world(char *worldpath, const unsigned char rotate, const int *wblimits);

void free_world(worldinfo *world);


#endif
