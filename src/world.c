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


static world measure_world(char *worldpath, const options *opts)
{
	world world;
	world.rcount = -1;

	// check for errors, strip trailing slash and append region directory
	size_t dirlen = strlen(worldpath);
	if (dirlen > WORLDDIR_PATH_MAXLEN)
	{
		fprintf(stderr, "World path is too long (max 255 characters)\n");
		return world;
	}
	if (worldpath[dirlen - 1] == '/')
		worldpath[dirlen - 1] = 0;
	sprintf(world.regiondir, "%s/region", worldpath);

	DIR *dir = opendir(world.regiondir);
	if (dir == NULL)
	{
		fprintf(stderr, "Error %d reading region directory: %s\n", errno, world.regiondir);
		return world;
	}

	int wrlimits[4], allrclimits[4];
	if (opts->use_limits)
		for (int i = 0; i < 4; i++)
		{
			// dividing rounds toward zero, so bit shift to get the floor instead
			wrlimits[i] = opts->limits[i] >> REGION_BLOCK_BITS;
			allrclimits[i] = opts->limits[i] >> CHUNK_BLOCK_BITS & MAX_REGION_CHUNK;
		}

	world.rcount = 0;

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
			if (opts->use_limits &&
					(rz < wrlimits[0] || rx > wrlimits[1] || rz > wrlimits[2] || rx < wrlimits[3]))
				continue;

			if (world.rcount == 0)
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
			world.rcount++;
		}
	}
	if (!world.rcount) return world;

	int rxsize = rxmax - rxmin + 1;
	int rzsize = rzmax - rzmin + 1;
	world.rrxsize = opts->rotate % 2 ? rzsize : rxsize;
	world.rrzsize = opts->rotate % 2 ? rxsize : rzsize;
	world.rrxmax = world.rrxsize - 1;
	world.rrzmax = world.rrzsize - 1;
	world.rotate = opts->rotate;

	// now that we have counted the regions, allocate an array
	// and loop through directory again to read the region data
	world.regionmap = (region**)calloc(world.rrxsize * world.rrzsize, sizeof(region*));
	world.regions = (region*)calloc(world.rcount, sizeof(region));

	int rclimits[4] = {0, MAX_REGION_CHUNK, MAX_REGION_CHUNK, 0};
	int r = 0;
	rewinddir(dir);
	while ((ent = readdir(dir)) != NULL)
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			if (opts->use_limits &&
					(rz < wrlimits[0] || rx > wrlimits[1] || rz > wrlimits[2] || rx < wrlimits[3]))
				continue;

			// get rotated world-relative region coords from absolute coords
			int rrx, rrz;
			switch(world.rotate) {
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
			if (opts->use_limits)
			{
				rclimits[0] = (rz == wrlimits[0] ? allrclimits[0] : 0);
				rclimits[1] = (rx == wrlimits[1] ? allrclimits[1] : MAX_REGION_CHUNK);
				rclimits[2] = (rz == wrlimits[2] ? allrclimits[2] : MAX_REGION_CHUNK);
				rclimits[3] = (rx == wrlimits[3] ? allrclimits[3] : 0);
			}

			world.regions[r] = read_region(world.regiondir, rx, rz, rclimits);
			if (world.regions[r].loaded)
				world.regionmap[rrz * world.rrxsize + rrx] = &world.regions[r];
			r++;
		}
	closedir(dir);

	return world;
}


static void free_world(world *world)
{
	free(world->regionmap);
	free(world->regions);
}


static region *get_region_from_coords(const world *world, const int rrx, const int rrz)
{
	// check if region is out of bounds
	if (rrx < 0 || rrx > world->rrxmax || rrz < 0 || rrz > world->rrzmax) return NULL;
	// check if region exists
	return world->regionmap[rrz * world->rrxsize + rrx];
}


static void get_world_margins(int *margins, const world *world, const char isometric)
{
	// initialize margins to maximum, and decrease them as regions are found
	if (isometric)
	{
		margins[1] = margins[3] = (world->rrxsize + world->rrzsize) * ISO_REGION_X_MARGIN;
		margins[0] = margins[2] = (world->rrxsize + world->rrzsize) * ISO_REGION_Y_MARGIN
				- ISO_BLOCK_TOP_HEIGHT + ISO_CHUNK_DEPTH;
	}
	else
		for (int i = 0; i < 4; i++) margins[i] = REGION_BLOCK_LENGTH;

	for (int rrz = 0; rrz <= world->rrzmax; rrz++)
	{
		for (int rrx = 0; rrx <= world->rrxmax; rrx++)
		{
			// skip regions not on the edge of the map
			if (!isometric && rrx > 0 && rrx < world->rrxmax && rrz > 0 && rrz < world->rrzmax)
				continue;

			// get region, or skip if it doesn't exist
			region *reg = get_region_from_coords(world, rrx, rrz);
			if (reg == NULL) continue;

			int rmargins[4];
			get_region_margins(rmargins, reg, world->rotate, isometric);

			if (isometric)
			{
				// isometric offsets for this region
				int rro[] =
				{
					(rrx + rrz)                                 * ISO_REGION_Y_MARGIN, // top
					(world->rrxmax - rrx + rrz)                 * ISO_REGION_X_MARGIN, // right
					(world->rrxmax - rrx + world->rrzmax - rrz) * ISO_REGION_Y_MARGIN, // bottom
					(rrx + world->rrzmax - rrz)                 * ISO_REGION_X_MARGIN, // left
				};

				for (int i = 0; i < 4; i++)
				{
					// add region offset in pixels; if it's lower, update the final world margin
					int rmargin = rmargins[i] + rro[i];
					if (rmargin < margins[i]) margins[i] = rmargin;
				}
			}
			else
			{
				// use margins for the specific edge(s) that this region touches
				if (rrz == 0             && rmargins[0] < margins[0]) margins[0] = rmargins[0];
				if (rrx == world->rrxmax && rmargins[1] < margins[1]) margins[1] = rmargins[1];
				if (rrz == world->rrzmax && rmargins[2] < margins[2]) margins[2] = rmargins[2];
				if (rrx == 0             && rmargins[3] < margins[3]) margins[3] = rmargins[3];
			}
		}
	}
}


void render_tiny_world_map(image *image, int wpx, int wpy, const world *world, const options *opts)
{
	int r = 0;
	for (int rrz = 0; rrz <= world->rrzmax; rrz++)
	{
		for (int rrx = 0; rrx <= world->rrxmax; rrx++)
		{
			// get region, or skip if it doesn't exist
			region *reg = get_region_from_coords(world, rrx, rrz);
			if (reg == NULL) continue;

			r++;
			printf("Rendering region %d/%d (%d,%d)...\n", r, world->rcount, reg->x, reg->z);

			int rpx = rrx * REGION_CHUNK_LENGTH + wpx;
			int rpy = rrz * REGION_CHUNK_LENGTH + wpy;

			render_tiny_region_map(image, rpx, rpy, reg, opts);
		}
	}
}


void render_world_map(image *image, int wpx, int wpy, const world *world, const textures *tex,
		const options *opts)
{
	int r = 0;
	// we need to render the regions in order from top to bottom for isometric view
	for (int rrz = 0; rrz <= world->rrzmax; rrz++)
	{
		for (int rrx = 0; rrx <= world->rrxmax; rrx++)
		{
			// get region, or skip if it doesn't exist
			region *reg = get_region_from_coords(world, rrx, rrz);
			if (reg == NULL) continue;

			// get rotated neighbouring region paths
			region *nregions[4] =
			{
				get_region_from_coords(world, rrx, rrz - 1),
				get_region_from_coords(world, rrx + 1, rrz),
				get_region_from_coords(world, rrx, rrz + 1),
				get_region_from_coords(world, rrx - 1, rrz),
			};

			r++;
			printf("Rendering region %d/%d (%d,%d)...\n", r, world->rcount, reg->x, reg->z);

			int rpx, rpy;
			if (opts->isometric)
			{
				// translate orthographic region coordinates to isometric pixel coordinates
				rpx = (rrx + world->rrzmax - rrz) * ISO_REGION_X_MARGIN + wpx;
				rpy = (rrx + rrz)                * ISO_REGION_Y_MARGIN + wpy;
			}
			else
			{
				rpx = rrx * REGION_BLOCK_LENGTH + wpx;
				rpy = rrz * REGION_BLOCK_LENGTH + wpy;
			}

			render_region_map(image, rpx, rpy, reg, nregions, tex, opts);
		}
	}
}


void save_tiny_world_map(char *worlddir, const char *imagefile, const options *opts)
{
	world world = measure_world(worlddir, opts);
	if (!world.rcount)
	{
		fprintf(stderr, "No regions found in directory: %s\n", worlddir);
		return;
	}

	int width, height, margins[4];
	get_world_margins(margins, &world, 0);

	// tiny map's scale is 1 chunk : 1 pixel
	for (int i = 0; i < 4; i++) margins[i] /= CHUNK_BLOCK_LENGTH;

	width  = world.rrxsize * REGION_CHUNK_LENGTH - margins[1] - margins[3];
	height = world.rrzsize * REGION_CHUNK_LENGTH - margins[0] - margins[2];

	image wimage = create_image(width, height);
	printf("Read %d regions. Image dimensions: %d x %d\n", world.rcount, width, height);

	render_tiny_world_map(&wimage, -margins[3], -margins[0], &world, opts);

	free_world(&world);

	printf("Saving image to %s ...\n", imagefile);
	save_image(wimage, imagefile);

	free_image(wimage);
}


void save_world_map(char *worldpath, const char *imagefile, const options *opts)
{
	world world = measure_world(worldpath, opts);

	// check for errors
	if (world.rcount == -1) return;
	if (world.rcount == 0)
	{
		fprintf(stderr, "No regions found in world region directory: %s\n", world.regiondir);
		return;
	}

	int width, height, margins[4];
	if (opts->isometric)
	{
		width  = (world.rrxsize + world.rrzsize) * ISO_REGION_X_MARGIN;
		height = (world.rrxsize + world.rrzsize) * ISO_REGION_Y_MARGIN
				- ISO_BLOCK_TOP_HEIGHT + ISO_CHUNK_DEPTH;
	}
	else
	{
		width  = world.rrxsize * REGION_BLOCK_LENGTH;
		height = world.rrzsize * REGION_BLOCK_LENGTH;
	}
	get_world_margins(margins, &world, opts->isometric);
	width  -= (margins[1] + margins[3]);
	height -= (margins[0] + margins[2]);

	image wimage = create_image(width, height);
	printf("Read %d regions. Image dimensions: %d x %d\n", world.rcount, width, height);

	textures *tex = read_textures(opts->texpath, opts->shapepath);

	clock_t start = clock();
	render_world_map(&wimage, -margins[3], -margins[0], &world, tex, opts);
	clock_t render_end = clock();
	printf("Total render time: %f seconds\n", (double)(render_end - start) / CLOCKS_PER_SEC);

	free_textures(tex);
	free_world(&world);

	printf("Saving image to %s ...\n", imagefile);
	save_image(wimage, imagefile);
	printf("Total save time: %f seconds\n", (double)(clock() - render_end) / CLOCKS_PER_SEC);

	free_image(wimage);
}
