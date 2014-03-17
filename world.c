#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lodepng.h"

#include "colours.h"
#include "region.h"


typedef struct world {
	const char* dir;
	int rcount, rxmin, rxmax, rzmin, rzmax, rxsize, rzsize;
	int margins[4];
	int* regions;
} world;


static void get_world_margins(world* world)
{
	// initialize margins to maximum, and decrease them as regions are parsed
	for (int i = 0; i < 4; i++)
	{
		world->margins[i] = REGION_BLOCK_WIDTH;
	}

	for (int r = 0; r < world->rcount; r++)
	{
		int rx = world->regions[r * 2];
		int rz = world->regions[r * 2 + 1];

		// check margins if this region is on the edge of the map
		if (rx == world->rxmin || rx == world->rxmax || rz == world->rzmin || rz == world->rzmax)
		{
			char path[255];
			// FIXME: path/filename joining needs to be more flexible
			sprintf(path, "%s/r.%d.%d.mca", world->dir, rx, rz);

			int rmargins[4];
			get_region_margins(path, rmargins);
			//printf("Margins for region %d, %d: N %d, E %d, S %d, W %d\n",
			//		rx, rz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

			// use margins for the specific edge(s) that this region touches
			if (rz == world->rzmin && rmargins[0] < world->margins[0]) // north
			{
				world->margins[0] = rmargins[0];
			}
			if (rx == world->rxmax && rmargins[1] < world->margins[1]) // east
			{
				world->margins[1] = rmargins[1];
			}
			if (rz == world->rzmax && rmargins[2] < world->margins[2]) // south
			{
				world->margins[2] = rmargins[2];
			}
			if (rx == world->rxmin && rmargins[3] < world->margins[3]) // west
			{
				world->margins[3] = rmargins[3];
			}
		}
	}
	//printf("Margins: N %d, E %d, S %d, W %d\n",
	//		world->margins[0], world->margins[1], world->margins[2], world->margins[3]);
}


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
			world.rcount++;
		}
	}
	if (!world.rcount) return world;

	// be sure to free this pointer later
	world.regions = (int*)malloc(world.rcount * sizeof(int) * 2);

	rewinddir(dir);
	int r = 0;
	while ((ent = readdir(dir)) != NULL)
	{
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			world.regions[r * 2] = rx;
			world.regions[r * 2 + 1] = rz;

			if (r == 0)
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

			r++;
		}
	}
	closedir(dir);

	world.rxsize = (world.rxmax - world.rxmin + 1);
	world.rzsize = (world.rzmax - world.rzmin + 1);

	get_world_margins(&world);

	return world;
}


unsigned char* render_world_blockmap(world world, const colour* colours,
		const char night)
{
	int w = world.rxsize * REGION_BLOCK_WIDTH - world.margins[1] - world.margins[3];
	int h = world.rzsize * REGION_BLOCK_WIDTH - world.margins[0] - world.margins[2];
	printf("Read %d regions. Image dimensions: %d x %d\n", world.rcount, w, h);
	unsigned char* worldimage = (unsigned char*)calloc(w * h * CHANNELS, sizeof(char));

	for (int r = 0; r < world.rcount; r++)
	{
		int rx = world.regions[r * 2];
		int rz = world.regions[r * 2 + 1];
		printf("Rendering region %d/%d at %d, %d\n", r + 1, world.rcount, rx, rz);

		char path[255];
		// FIXME: path/filename joining needs to be more flexible
		sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);
		unsigned char* regionimage = render_region_blockmap(path, colours, night);

		int rxoffset = (rx == world.rxmin ? world.margins[3] : 0);
		int rpx = (rx - world.rxmin) * REGION_BLOCK_WIDTH - world.margins[3] + rxoffset;
		int rw = REGION_BLOCK_WIDTH - rxoffset - (rx == world.rxmax ? world.margins[1] : 0);

		int rzoffset = (rz == world.rzmin ? world.margins[0] : 0);
		int rpz = (rz - world.rzmin) * REGION_BLOCK_WIDTH - world.margins[0] + rzoffset;
		int rh = REGION_BLOCK_WIDTH - rzoffset - (rz == world.rzmax ? world.margins[2] : 0);

		for (int z = 0; z < rh; z++)
		{
			// copy a line of pixel data from the region image to the world image
			memcpy(&worldimage[((rpz + z) * w + rpx) * CHANNELS],
					&regionimage[((rzoffset + z) * REGION_BLOCK_WIDTH + rxoffset) * CHANNELS],
					rw * CHANNELS);
		}
		free(regionimage);
	}

	return worldimage;
}


unsigned char* render_world_iso_blockmap(world world, const colour* colours,
		const char night)
{
	unsigned int side = (world.rxmax - world.rxmin + world.rzmax - world.rzmin + 2) / 2;
	unsigned int w = side * ISO_REGION_WIDTH;
	unsigned int h = side * (REGION_BLOCK_WIDTH - 1) * 2 * ISO_BLOCK_STEP
			+ ISO_BLOCK_HEIGHT * CHUNK_BLOCK_HEIGHT;

	printf("Read %d regions. Image dimensions: %d x %d\n", world.rcount, w, h);
	unsigned char* worldimage = (unsigned char*)calloc(w * h * CHANNELS, sizeof(char));

	int rc = 0;
	// we need to render the regions in a certain order
	for (int rz = world.rzmin; rz <= world.rzmax; rz++)
	{
		for (int rx = world.rxmax; rx >= world.rxmin; rx--)
		{
			// only read the region if we know it exists
			int exists = 0;
			for (int i = 0; i < world.rcount; i++)
			{
				if (world.regions[i * 2] == rx && world.regions[i * 2 + 1] == rz)
				{
					exists = 1;
					break;
				}
			}
			if (!exists) continue;

			rc++;
			int rwx = (rx - world.rxmin);
			int rwz = (rz - world.rzmin);
			int rpx = (rwx + rwz) * ISO_REGION_WIDTH / 2;
			int rpy = (world.rxsize - rwx + rwz - 1) * REGION_BLOCK_WIDTH * ISO_BLOCK_STEP;
			printf("Rendering region %d/%d (%d,%d) at pixel %d,%d\n",
					rc, world.rcount, rx, rz, rpx, rpy);

			char path[255];
			sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);
			unsigned char* regionimage = render_region_iso_blockmap(path, colours, night);

			for (int py = 0; py < ISO_REGION_HEIGHT; py++)
			{
				for (int px = 0; px < ISO_REGION_WIDTH; px++)
				{
					combine_alpha(&regionimage[(py * ISO_REGION_WIDTH + px) * CHANNELS],
							&worldimage[((rpy + py) * w + rpx + px) * CHANNELS], 1);
				}
			}
			free(regionimage);
		}
	}

	return worldimage;
}


void save_world_blockmap(const char* worlddir, const char* imagefile, const colour* colours,
		const char night, const char isometric)
{
	world world = measure_world(worlddir);
	if (!world.rcount)
	{
		printf("No regions found in directory: %s\n", worlddir);
		return;
	}

	unsigned int w, h;
	unsigned char* worldimage;
	if (isometric)
	{
		unsigned int side = (world.rxmax - world.rxmin + world.rzmax - world.rzmin + 2) / 2;
		w = side * ISO_REGION_WIDTH;
		h = side * (REGION_BLOCK_WIDTH - 1) * 2 * ISO_BLOCK_STEP
				+ ISO_BLOCK_HEIGHT * CHUNK_BLOCK_HEIGHT;
		worldimage = render_world_iso_blockmap(world, colours, night);
	}
	else
	{
		w = h = REGION_BLOCK_WIDTH;
		worldimage = render_world_blockmap(world, colours, night);
	}
	if (worldimage == NULL) return;

	free(world.regions);

	printf("Saving image to %s ...\n", imagefile);
	lodepng_encode32_file(imagefile, worldimage, w, h);
	free(worldimage);
}
