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
#include <time.h>

#include "chunk.h"
#include "dims.h"
#include "image.h"
#include "region.h"
#include "textures.h"
#include "world.h"


static world measure_world(const char* worlddir)
{
	world world;
	world.dir = worlddir;
	world.rcount = 0;

	DIR* dir = opendir(worlddir);
	if (dir == NULL)
	{
		printf("Error %d reading region directory: %s\n", errno, worlddir);
		return world;
	}

	// list directory to find the number of region files, and image dimensions
	struct dirent* ent;
	int rx, rz, length;
	char ext[4]; // three-char extension + null char
	while ((ent = readdir(dir)) != NULL)
	{
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			if (world.rcount == 0)
			{
				world.rxmin = world.rxmax = rx;
				world.rzmin = world.rzmax = rz;
			}
			else
			{
				if (rx < world.rxmin) world.rxmin = rx;
				if (rx > world.rxmax) world.rxmax = rx;
				if (rz < world.rzmin) world.rzmin = rz;
				if (rz > world.rzmax) world.rzmax = rz;
			}
			world.rcount++;
		}
	}
	if (!world.rcount) return world;

	world.rxsize = world.rxmax - world.rxmin + 1;
	world.rzsize = world.rzmax - world.rzmin + 1;

	// be sure to free this pointer later
	world.regionmap = (unsigned char*)calloc(world.rxsize * world.rzsize, sizeof(char));

	rewinddir(dir);
	while ((ent = readdir(dir)) != NULL)
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
			world.regionmap[(rz - world.rzmin) * world.rxsize + rx - world.rxmin] = 1;
	closedir(dir);

	return world;
}


static void free_world(world world)
{
	free(world.regionmap);
}


static void get_path_from_rel_coords(char* path, const world world, const int rwx, const int rwz,
		const char rotate)
{
	// check if region is out of bounds
	if (rwx < 0 || rwz < 0) return;
	if (!(rotate % 2) && (rwx > world.rxsize - 1 || rwz > world.rzsize - 1)) return;
	if (rotate % 2 && (rwx > world.rzsize - 1 || rwz > world.rxsize - 1)) return;

	// get absolute region coords from rotated world-relative coords
	int rx, rz;
	switch(rotate) {
	case 0:
		rx = rwx + world.rxmin;
		rz = rwz + world.rzmin;
		break;
	case 1:
		rx = rwz + world.rxmin;
		rz = (world.rzsize - rwx - 1) + world.rzmin;
		break;
	case 2:
		rx = (world.rxsize - rwx - 1) + world.rxmin;
		rz = (world.rzsize - rwz - 1) + world.rzmin;
		break;
	case 3:
		rx = (world.rxsize - rwz - 1) + world.rxmin;
		rz = rwx + world.rzmin;
		break;
	}

	if (!world.regionmap[(rz - world.rzmin) * world.rxsize + rx - world.rxmin]) return;
	// FIXME: path/filename joining needs to be more flexible
	sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);
}


static void get_world_margins(world world, int* margins, const char rotate)
{
	int rwxsize = rotate % 2 ? world.rzsize : world.rxsize;
	int rwzsize = rotate % 2 ? world.rxsize : world.rzsize;

	// initialize margins to maximum, and decrease them as regions are found
	for (int i = 0; i < 4; i++) margins[i] = REGION_BLOCK_LENGTH;

	for (int rwz = 0; rwz < rwzsize; rwz++)
	{
		for (int rwx = 0; rwx < rwxsize; rwx++)
		{
			// skip regions not on the edge of the map
			if (rwx > 0 && rwx < rwxsize - 1 && rwz > 0 && rwz < rwzsize - 1) continue;

			char path[255] = "";
			get_path_from_rel_coords(path, world, rwx, rwz, rotate);
			if (!strcmp(path, "")) continue;

			int rmargins[4];
			get_region_margins(path, rmargins, rotate);
			//printf("Margins for region %d, %d: top %d, right %d, bottom %d, left %d\n",
			//		rx, rz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

			// use margins for the specific edge(s) that this region touches
			if (rwz == 0 && rmargins[0] < margins[0]) margins[0] = rmargins[0];
			if (rwx == rwxsize - 1 && rmargins[1] < margins[1]) margins[1] = rmargins[1];
			if (rwz == rwzsize - 1 && rmargins[2] < margins[2]) margins[2] = rmargins[2];
			if (rwx == 0 && rmargins[3] < margins[3]) margins[3] = rmargins[3];
		}
	}

	//printf("Margins: top %d, right %d, bottom %d, left %d\n",
	//		margins[0], margins[1], margins[2], margins[3]);
}


static void get_world_iso_margins(world world, int* margins, const char rotate)
{
	int rwxsize = rotate % 2 ? world.rzsize : world.rxsize;
	int rwzsize = rotate % 2 ? world.rxsize : world.rzsize;

	// initialize margins to maximum, and decrease them as regions are found
	margins[0] = margins[2] = (world.rxsize + world.rzsize) * ISO_REGION_Y_MARGIN
			- ISO_BLOCK_TOP_HEIGHT;
	margins[1] = margins[3] = (world.rxsize + world.rzsize + 1) * ISO_REGION_X_MARGIN;

	// world margins measured in regions - used only so we can skip irrelevant regions
	int wrm[4];
	for (int i = 0; i < 4; i++) wrm[i] = rwxsize + rwzsize;

	for (int rwz = 0; rwz < rwzsize; rwz++)
	{
		for (int rwx = 0; rwx < rwxsize; rwx++)
		{
			char path[255] = "";
			get_path_from_rel_coords(path, world, rwx, rwz, rotate);
			if (!strcmp(path, "")) continue;

			// rotated margins for this region, measured in regions - compare to wrm
			int rrm[] = {
					rwx + rwz, // top
					rwxsize - 1 - rwx + rwz, // right
					rwxsize - 1 - rwx + rwzsize - 1 - rwz, // bottom
					rwx + rwzsize - 1 - rwz, // left
			};

			// skip reading this region unless it's the closest yet to one or more corners
			if (rrm[0] < wrm[0] || rrm[1] < wrm[1] || rrm[2] < wrm[2] || rrm[3] < wrm[3])
			{
				int rmargins[4];
				get_region_iso_margins(path, rmargins, rotate);
				//printf("Margins for region at %d,%d: T %d, R %d, B %d, L %d\n",
				//		rwx, rwz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

				int rmargin;
				for (int i = 0; i < 4; i++)
				{
					// skip this corner unless it's one that's close to a world corner
					if (rrm[i] <= wrm[i])
					{
						wrm[i] = rrm[i];
						// margin for this corner of this region in pixels
						// assign to world margin if lower
						rmargin = rrm[i] * (i % 2 ? ISO_REGION_X_MARGIN : ISO_REGION_Y_MARGIN)
								+ rmargins[i];
						if (rmargin < margins[i]) margins[i] = rmargin;
					}
				}
			}
		}
	}

	//printf("World margins in regions: top %d, right %d, bottom %d, left %d\n",
	//		wrm[0], wrm[1], wrm[2], wrm[3]);
	//printf("World margins in pixels: top %d, right %d, bottom %d, left %d\n",
	//		margins[0], margins[1], margins[2], margins[3]);
}


void render_world_map(image* image, int wpx, int wpy, world world, const textures* tex,
		const options opts)
{
	int rwxsize = opts.rotate % 2 ? world.rzsize : world.rxsize;
	int rwzsize = opts.rotate % 2 ? world.rxsize : world.rzsize;

	int r = 0;
	// we need to render the regions in a certain order
	for (int rwz = 0; rwz < rwzsize; rwz++)
	{
		for (int rwx = 0; rwx < rwxsize; rwx++)
		{
			// get region file path, or skip if it doesn't exist
			char path[255] = "";
			get_path_from_rel_coords(path, world, rwx, rwz, opts.rotate);
			if (!strcmp(path, "")) continue;

			// get rotated neighbouring region paths
			char *npaths[4];
			for (int i = 0; i < 4; i++) npaths[i] = (char*)calloc(255, sizeof(char));
			get_path_from_rel_coords(npaths[0], world, rwx, rwz - 1, opts.rotate);
			get_path_from_rel_coords(npaths[1], world, rwx + 1, rwz, opts.rotate);
			get_path_from_rel_coords(npaths[2], world, rwx, rwz + 1, opts.rotate);
			get_path_from_rel_coords(npaths[3], world, rwx - 1, rwz, opts.rotate);

			r++;
			printf("Rendering region %d/%d...\n", r, world.rcount);

			int rpx, rpy;
			if (opts.isometric)
			{
				// translate orthographic region coordinates to isometric pixel coordinates
				rpx = (rwx + rwzsize - 1 - rwz) * ISO_REGION_X_MARGIN + wpx;
				rpy = (rwx + rwz) * ISO_REGION_Y_MARGIN + wpy;
			}
			else
			{
				rpx = rwx * REGION_BLOCK_LENGTH + wpx;
				rpy = rwz * REGION_BLOCK_LENGTH + wpy;
			}

			render_region_map(image, rpx, rpy, path, npaths, tex, opts);

			for (int i = 0; i < 4; i++) free(npaths[i]);
		}
	}
}


void save_world_map(const char* worlddir, const char* imagefile, const textures* tex,
		const options opts)
{
	world world = measure_world(worlddir);
	if (!world.rcount)
	{
		printf("No regions found in directory: %s\n", worlddir);
		return;
	}

	int rwxsize = opts.rotate % 2 ? world.rzsize : world.rxsize;
	int rwzsize = opts.rotate % 2 ? world.rxsize : world.rzsize;
	int width, height, margins[4];
	if (opts.isometric)
	{
		get_world_iso_margins(world, margins, opts.rotate);
		width = (rwxsize + rwzsize) * ISO_REGION_X_MARGIN;
		height = ((rwxsize + rwzsize) * ISO_REGION_Y_MARGIN - ISO_BLOCK_TOP_HEIGHT)
				+ ISO_CHUNK_DEPTH;
	}
	else
	{
		get_world_margins(world, margins, opts.rotate);
		width = rwxsize * REGION_BLOCK_LENGTH;
		height = rwzsize * REGION_BLOCK_LENGTH;
	}

	image wimage = create_image(width - margins[1] - margins[3], height - margins[0] - margins[2]);
	printf("Read %d regions. Image dimensions: %d x %d\n",
			world.rcount, wimage.width, wimage.height);

	clock_t start = clock();
	render_world_map(&wimage, -margins[3], -margins[0], world, tex, opts);
	clock_t render_end = clock();
	printf("Total render time: %f seconds\n", (double)(render_end - start) / CLOCKS_PER_SEC);

	free_world(world);

	printf("Saving image to %s ...\n", imagefile);
	save_image(wimage, imagefile);
	printf("Total save time: %f seconds\n", (double)(clock() - render_end) / CLOCKS_PER_SEC);

	free_image(wimage);
}
