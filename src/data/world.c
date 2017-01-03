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


#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"


region *get_region_from_coords(const worldinfo *world, const uint32_t rrx, const uint32_t rrz)
{
	// check if region is out of bounds
	if (rrx < 0 || rrx > world->rrxmax || rrz < 0 || rrz > world->rrzmax) return NULL;
	// check if region exists
	return world->regionmap[rrz * world->rrxsize + rrx];
}


worldinfo *measure_world(char *worldpath, const uint8_t rotate, const int32_t *wblimits,
	const bool nether, const bool end)
{
	worldinfo *world = (worldinfo*)malloc(sizeof(worldinfo));
	world->rcount = 0;

	// check for errors, strip trailing slash and append region directory
	size_t dirlen = strlen(worldpath);
	if (dirlen > WORLDDIR_PATH_MAXLEN)
	{
		fprintf(stderr, "World path is too long (max 255 characters)\n");
		free_world(world);
		return NULL;
	}
	if (worldpath[dirlen - 1] == '/')
		worldpath[dirlen - 1] = 0;
	sprintf(world->regiondir, "%s%s/region", worldpath, nether ? "/DIM-1" : (end ? "/DIM1" : ""));

	DIR *dir = opendir(world->regiondir);
	if (dir == NULL)
	{
		fprintf(stderr, "Error %d reading region directory: %s\n", errno, world->regiondir);
		free_world(world);
		return NULL;
	}

	int32_t wrlimits[4];
	int16_t wrblimits[4];
	if (wblimits != NULL)
		for (uint8_t i = 0; i < 4; i++)
		{
			// dividing rounds toward zero, so bit shift to get the floor instead
			wrlimits[i] = wblimits[i] >> REGION_BLOCK_BITS;
			wrblimits[i] = wblimits[i] & MAX_REGION_BLOCK;
		}

	// list directory to find the number of region files, and image dimensions
	struct dirent *ent;
	int32_t rx, rz, rxmin, rxmax, rzmin, rzmax, length;
	char ext[4]; // three-char extension + null char
	while ((ent = readdir(dir)) != NULL)
	{
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			if (wblimits != NULL && (
					rz < wrlimits[NORTH] || rx > wrlimits[EAST] ||
					rz > wrlimits[SOUTH] || rx < wrlimits[WEST]))
				continue;

			if (world->rcount == 0)
			{
				rxmin = rxmax = rx;
				rzmin = rzmax = rz;
			}
			else
			{
				if (rx < rxmin) rxmin = rx;
				if (rx > rxmax) rxmax = rx;
				if (rz < rzmin) rzmin = rz;
				if (rz > rzmax) rzmax = rz;
			}
			world->rcount++;
		}
	}
	if (!world->rcount)
	{
		fprintf(stderr, "No regions found in world region directory: %s\n", world->regiondir);
		free_world(world);
		return NULL;
	}

	uint32_t rxsize = rxmax - rxmin + 1;
	uint32_t rzsize = rzmax - rzmin + 1;
	world->rrxsize = rotate % 2 ? rzsize : rxsize;
	world->rrzsize = rotate % 2 ? rxsize : rzsize;
	world->rrxmax = world->rrxsize - 1;
	world->rrzmax = world->rrzsize - 1;
	world->rotate = rotate;

	// now that we have counted the regions, allocate an array
	// and loop through directory again to read the region data
	world->regionmap = (region**)calloc(world->rrxsize * world->rrzsize, sizeof(region*));

	rewinddir(dir);
	while ((ent = readdir(dir)) != NULL)
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			uint16_t *rblimits;

			// get chunk limits for this region
			if (wblimits != NULL)
			{
				if (rz < wrlimits[NORTH] || rx > wrlimits[EAST] ||
					rz > wrlimits[SOUTH] || rx < wrlimits[WEST])
					continue;

				rblimits = (uint16_t*)malloc(4 * sizeof(uint16_t));
				rblimits[NORTH] = (rz == wrlimits[NORTH] ? wrblimits[NORTH] : 0);
				rblimits[EAST]  = (rx == wrlimits[EAST]  ? wrblimits[EAST]  : MAX_REGION_BLOCK);
				rblimits[SOUTH] = (rz == wrlimits[SOUTH] ? wrblimits[SOUTH] : MAX_REGION_BLOCK);
				rblimits[WEST]  = (rx == wrlimits[WEST]  ? wrblimits[WEST]  : 0);
			}
			else rblimits = NULL;

			region *reg = read_region(world->regiondir, rx, rz, rblimits);
			free(rblimits);

			if (reg == NULL) continue;

			// get rotated world-relative region coords from absolute coords
			uint32_t rrx, rrz;
			switch(world->rotate) {
			case 0:
				rrx = rx - rxmin;
				rrz = rz - rzmin;
				break;
			case 1:
				rrx = rzmax - rz;
				rrz = rx - rxmin;
				break;
			case 2:
				rrx = rxmax - rx;
				rrz = rzmax - rz;
				break;
			case 3:
				rrx = rz - rzmin;
				rrz = rxmax - rx;
				break;
			}
			world->regionmap[rrz * world->rrxsize + rrx] = reg;
		}
	closedir(dir);

	return world;
}


void free_world(worldinfo *world)
{
	for (uint32_t i = 0; i < world->rrxsize * world->rrzsize; i++)
		if (world->regionmap[i] != NULL) free_region(world->regionmap[i]);
	free(world->regionmap);
	free(world);
}
