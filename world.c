#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lodepng.h"

#include "colours.h"
#include "region.h"


typedef struct world {
	int rcount;
	int rxmin, rxmax, rzmin, rzmax;
	int margins[4];
} world;


static world measure_world(const char* worlddir)
{
	world world;
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
	char path[255];
	while ((ent = readdir(dir)) != NULL)
	{
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			sprintf(path, "%s/%s", worlddir, ent->d_name);
			if (!world.rcount)
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

	// initialize margins to maximum, and decrease them as regions are parsed
	int rmargins[4];
	for (int i = 0; i < 4; i++)
	{
		world.margins[i] = REGION_BLOCK_WIDTH;
	}

	rewinddir(dir);
	while ((ent = readdir(dir)) != NULL)
	{
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			// check margins if this region is on the edge of the map
			if (rx == world.rxmin || rx == world.rxmax || rz == world.rzmin || rz == world.rzmax)
			{
				sprintf(path, "%s/%s", worlddir, ent->d_name);
				get_region_margins(path, rmargins);
				//printf("Margins for region %d, %d: N %d, E %d, S %d, W %d\n",
				//		rx, rz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

				// use margins for the specific edge(s) that this region touches
				if (rz == world.rzmin && rmargins[0] < world.margins[0]) // north
				{
					world.margins[0] = rmargins[0];
				}
				if (rx == world.rxmax && rmargins[1] < world.margins[1]) // east
				{
					world.margins[1] = rmargins[1];
				}
				if (rz == world.rzmax && rmargins[2] < world.margins[2]) // south
				{
					world.margins[2] = rmargins[2];
				}
				if (rx == world.rxmin && rmargins[3] < world.margins[3]) // west
				{
					world.margins[3] = rmargins[3];
				}
			}
		}
	}
	//printf("Margins: N %d, E %d, S %d, W %d\n",
	//		world.margins[0], world.margins[1], world.margins[2], world.margins[3]);

	closedir(dir);

	return world;
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

	int w = (world.rxmax - world.rxmin + 1) * REGION_BLOCK_WIDTH
			- world.margins[1] - world.margins[3];
	int h = (world.rzmax - world.rzmin + 1) * REGION_BLOCK_WIDTH
			- world.margins[0] - world.margins[2];
	printf("Read %d regions. Image dimensions: %d x %d\n", world.rcount, w, h);
	unsigned char* worldimage = (unsigned char*)malloc(w * h * CHANNELS);

	DIR* dir = opendir(worlddir);
	if (dir == NULL)
	{
		printf("Error %d reading region directory: %s\n", errno, worlddir);
		return;
	}
	// create an array of region files and
	struct dirent* ent;
	int rx, rz, length;
	char ext[4]; // three-char extension + null char
	rewinddir(dir);
	char path[255];
	int rc = 0;
	while ((ent = readdir(dir)) != NULL)
	{
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			// TODO: add support for .mcr files

			rc++;
			printf("Rendering region %d/%d at %d, %d\n", rc, world.rcount, rx, rz);

			// FIXME: path/filename joining needs to be more flexible
			sprintf(path, "%s/%s", worlddir, ent->d_name);

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
	}
	closedir(dir);

	printf("Saving image to %s ...\n", imagefile);
	lodepng_encode32_file(imagefile, worldimage, w, h);
	free(worldimage);
}
