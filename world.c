#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lodepng.h"

#include "colours.h"
#include "region.h"
#include "world.h"


static void get_world_margins(world world, int* margins)
{
	// initialize margins to maximum, and decrease them as regions are parsed
	for (int i = 0; i < 4; i++)
	{
		margins[i] = REGION_BLOCK_LENGTH;
	}

	for (int r = 0; r < world.rcount; r++)
	{
		int rx = world.regions[r * 2];
		int rz = world.regions[r * 2 + 1];

		// check margins if this region is on the edge of the map
		if (rx == world.rxmin || rx == world.rxmax || rz == world.rzmin || rz == world.rzmax)
		{
			char path[255];
			// FIXME: path/filename joining needs to be more flexible
			sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);

			int rmargins[4];
			get_region_margins(path, rmargins);
			//printf("Margins for region %d, %d: N %d, E %d, S %d, W %d\n",
			//		rx, rz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

			// use margins for the specific edge(s) that this region touches
			if (rz == world.rzmin && rmargins[0] < margins[0]) margins[0] = rmargins[0]; // north
			if (rx == world.rxmax && rmargins[1] < margins[1]) margins[1] = rmargins[1]; // east
			if (rz == world.rzmax && rmargins[2] < margins[2]) margins[2] = rmargins[2]; // south
			if (rx == world.rxmin && rmargins[3] < margins[3]) margins[3] = rmargins[3]; // west
		}
	}
	//printf("Margins: N %d, E %d, S %d, W %d\n",
	//		margins[0], margins[1], margins[2], margins[3]);
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

	return world;
}


image render_world_blockmap(world world, const colour* colours, const char night)
{
	int margins[4];
	get_world_margins(world, margins);

	image wimage;
	wimage.width = world.rxsize * REGION_BLOCK_LENGTH - margins[1] - margins[3];
	wimage.height = world.rzsize * REGION_BLOCK_LENGTH - margins[0] - margins[2];
	wimage.data = (unsigned char*)calloc(wimage.width * wimage.height * CHANNELS, sizeof(char));
	printf("Read %d regions. Image dimensions: %d x %d\n",
			world.rcount, wimage.width, wimage.height);

	for (int r = 0; r < world.rcount; r++)
	{
		int rx = world.regions[r * 2];
		int rz = world.regions[r * 2 + 1];
		printf("Rendering region %d/%d at %d, %d\n", r + 1, world.rcount, rx, rz);

		char path[255];
		// FIXME: path/filename joining needs to be more flexible
		sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);
		image rimage = render_region_blockmap(path, colours, night);

		int rxoffset = (rx == world.rxmin ? margins[3] : 0);
		int rpx = (rx - world.rxmin) * REGION_BLOCK_LENGTH - margins[3] + rxoffset;
		int rw = REGION_BLOCK_LENGTH - rxoffset - (rx == world.rxmax ? margins[1] : 0);

		int rzoffset = (rz == world.rzmin ? margins[0] : 0);
		int rpz = (rz - world.rzmin) * REGION_BLOCK_LENGTH - margins[0] + rzoffset;
		int rh = REGION_BLOCK_LENGTH - rzoffset - (rz == world.rzmax ? margins[2] : 0);

		for (int z = 0; z < rh; z++)
		{
			// copy a line of pixel data from the region image to the world image
			memcpy(&wimage.data[((rpz + z) * wimage.width + rpx) * CHANNELS],
					&rimage.data[((rzoffset + z) * REGION_BLOCK_LENGTH + rxoffset) * CHANNELS],
					rw * CHANNELS);
		}
		free(rimage.data);
	}

	return wimage;
}


image render_world_iso_blockmap(world world, const colour* colours, const char night)
{
	int margins[4];
	get_world_margins(world, margins);

	image wimage;
	wimage.width = (world.rxsize + world.rzsize) * ISO_REGION_WIDTH / 2;
	wimage.height = ((world.rxsize + world.rzsize) * REGION_BLOCK_LENGTH - 1) * ISO_BLOCK_STEP
			+ ISO_CHUNK_DEPTH;
	wimage.data = (unsigned char*)calloc(wimage.width * wimage.height * CHANNELS, sizeof(char));
	printf("Read %d regions. Image dimensions: %d x %d\n",
			world.rcount, wimage.width, wimage.height);

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
			int rpy = (world.rxsize - rwx + rwz - 1) * REGION_BLOCK_LENGTH * ISO_BLOCK_STEP;
			printf("Rendering region %d/%d (%d,%d) at pixel %d,%d\n",
					rc, world.rcount, rx, rz, rpx, rpy);

			char path[255];
			sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);
			image rimage = render_region_iso_blockmap(path, colours, night);

			for (int py = 0; py < ISO_REGION_HEIGHT; py++)
			{
				for (int px = 0; px < ISO_REGION_WIDTH; px++)
				{
					combine_alpha(&rimage.data[(py * ISO_REGION_WIDTH + px) * CHANNELS],
							&wimage.data[((rpy + py) * wimage.width + rpx + px) * CHANNELS], 1);
				}
			}
			free(rimage.data);
		}
	}

	return wimage;
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

	image wimage = isometric
			? render_world_iso_blockmap(world, colours, night)
			: render_world_blockmap(world, colours, night);
	free(world.regions);

	printf("Saving image to %s ...\n", imagefile);
	lodepng_encode32_file(imagefile, wimage.data, wimage.width, wimage.height);
	free(wimage.data);
}
