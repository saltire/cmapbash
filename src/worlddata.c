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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunkmap.h"
#include "dims.h"
#include "worlddata.h"


worldinfo *measure_world(char *worldpath, const unsigned char rotate, const int *wblimits)
{
	worldinfo *world = (worldinfo*)malloc(sizeof(worldinfo));
	world->rcount = 0;

	// check for errors, strip trailing slash and append region directory
	size_t dirlen = strlen(worldpath);
	if (dirlen > WORLDDIR_PATH_MAXLEN)
	{
		fprintf(stderr, "World path is too long (max 255 characters)\n");
		return world;
	}
	if (worldpath[dirlen - 1] == '/')
		worldpath[dirlen - 1] = 0;
	sprintf(world->regiondir, "%s/region", worldpath);

	DIR *dir = opendir(world->regiondir);
	if (dir == NULL)
	{
		fprintf(stderr, "Error %d reading region directory: %s\n", errno, world->regiondir);
		return world;
	}

	int wrlimits[4], wrblimits[4];
	if (wblimits != NULL)
		for (int i = 0; i < 4; i++)
		{
			// dividing rounds toward zero, so bit shift to get the floor instead
			wrlimits[i] = wblimits[i] >> REGION_BLOCK_BITS;
			wrblimits[i] = wblimits[i] & MAX_REGION_BLOCK;
		}

	// list directory to find the number of region files, and image dimensions
	struct dirent *ent;
	int rx, rz, rxmin, rxmax, rzmin, rzmax, length;
	char ext[4]; // three-char extension + null char
	while ((ent = readdir(dir)) != NULL)
	{
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			if (wblimits != NULL &&
					(rz < wrlimits[0] || rx > wrlimits[1] || rz > wrlimits[2] || rx < wrlimits[3]))
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
		return world;
	}

	int rxsize = rxmax - rxmin + 1;
	int rzsize = rzmax - rzmin + 1;
	world->rrxsize = rotate % 2 ? rzsize : rxsize;
	world->rrzsize = rotate % 2 ? rxsize : rzsize;
	world->rrxmax = world->rrxsize - 1;
	world->rrzmax = world->rrzsize - 1;
	world->rotate = rotate;

	// now that we have counted the regions, allocate an array
	// and loop through directory again to read the region data
	world->regionmap = (region**)calloc(world->rrxsize * world->rrzsize, sizeof(region*));
	world->regions = (region*)calloc(world->rcount, sizeof(region));

	unsigned int rclimits[4] = {0, MAX_REGION_CHUNK, MAX_REGION_CHUNK, 0};
	unsigned int rblimits[4] = {0, MAX_REGION_BLOCK, MAX_REGION_BLOCK, 0};
	int r = 0;
	rewinddir(dir);
	while ((ent = readdir(dir)) != NULL)
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			if (wblimits != NULL &&
					(rz < wrlimits[0] || rx > wrlimits[1] || rz > wrlimits[2] || rx < wrlimits[3]))
				continue;

			// get rotated world-relative region coords from absolute coords
			int rrx, rrz;
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

			// get chunk limits for this region
			if (wblimits != NULL)
			{
				rblimits[0] = (rz == wrlimits[0] ? wrblimits[0] : 0);
				rblimits[1] = (rx == wrlimits[1] ? wrblimits[1] : MAX_REGION_BLOCK);
				rblimits[2] = (rz == wrlimits[2] ? wrblimits[2] : MAX_REGION_BLOCK);
				rblimits[3] = (rx == wrlimits[3] ? wrblimits[3] : 0);
			}

			world->regions[r] = read_region(world->regiondir, rx, rz, rblimits);
			if (world->regions[r].loaded)
				world->regionmap[rrz * world->rrxsize + rrx] = &world->regions[r];
			r++;
		}
	closedir(dir);

	return world;
}


void free_world(worldinfo *world)
{
	free(world->regionmap);
	free(world->regions);
	free(world);
}


region *get_region_from_coords(const worldinfo *world, const int rrx, const int rrz)
{
	// check if region is out of bounds
	if (rrx < 0 || rrx > world->rrxmax || rrz < 0 || rrz > world->rrzmax) return NULL;
	// check if region exists
	return world->regionmap[rrz * world->rrxsize + rrx];
}
