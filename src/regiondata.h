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


#ifndef REGIONDATA_H_
#define REGIONDATA_H_


#include "dims.h"


typedef struct region
{
	int x, z;
	char path[REGIONFILE_PATH_MAXLEN];
	unsigned int offsets[REGION_CHUNK_AREA];
	unsigned char loaded;
	unsigned int blimits[4], climits[4];
	FILE *file;
}
region;


FILE *open_region_file(region *reg);
void close_region_file(region *reg);

region read_region(const char *regiondir, const int rx, const int rz,
		const unsigned int rblimits[4]);

unsigned int get_chunk_offset(const unsigned int rcx, const unsigned int rcz,
		const char rotate);

char chunk_exists(const region *reg, const unsigned int rcx, const unsigned int rcz,
		const unsigned char rotate);
nbt_node *read_chunk(const region *reg, const int rcx, const int rcz, const char rotate);


#endif