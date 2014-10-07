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


#ifndef WORLDDATA_H_
#define WORLDDATA_H_


#include "dims.h"
#include "regiondata.h"


typedef struct worldinfo
{
	char regiondir[REGIONDIR_PATH_MAXLEN];
	unsigned int rcount, rrxsize, rrzsize, rrxmax, rrzmax;
	unsigned char rotate;
	region *regions;
	region **regionmap;
}
worldinfo;


worldinfo *measure_world(char *worldpath, const unsigned char rotate, const int *wblimits);
void free_world(worldinfo *world);

region *get_region_from_coords(const worldinfo *world, const int rrx, const int rrz);


#endif
