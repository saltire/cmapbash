#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world.h"


void save_world_blockmap(const char* worlddir, const char* imagefile, const colour* colours,
		const char alpha)
{
	DIR* dir = opendir(worlddir);
	if (dir == NULL)
	{
		printf("Error %d reading region directory: %s\n", errno, worlddir);
		return;
	}

	// list directory to find the number of region files, and image dimensions
	int count = 0;
	int rx, rz;
	int rxmin, rxmax, rzmin, rzmax;
	struct dirent* ent;
	while ((ent = readdir(dir)) != NULL)
	{
		if (sscanf(ent->d_name, "r.%d.%d.mca", &rx, &rz))
		{
			if (!count)
			{
				rxmin = rxmax = rx;
				rzmin = rzmax = rz;
			}
			else
			{
				rxmin = rx < rxmin ? rx : rxmin;
				rxmax = rx > rxmax ? rx : rxmax;
				rzmin = rz < rzmin ? rz : rzmin;
				rzmax = rz > rzmax ? rz : rzmax;
			}
			count++;
		}
	}

	if (count == 0)
	{
		printf("No regions found in directory: %s\n", worlddir);
		return;
	}

	// TODO: find region crop dimensions

	int w = (rxmax - rxmin + 1) * REGION_BLOCK_WIDTH;
	int h = (rzmax - rzmin + 1) * REGION_BLOCK_WIDTH;
	printf("Read %d regions. Image dimensions: %d x %d\n", count, w, h);
	unsigned char* worldimage = (unsigned char*)malloc(w * h * CHANNELS);

	// create an array of region files and
	rewinddir(dir);
	int rc = 0;
	while ((ent = readdir(dir)) != NULL)
	{
		char ext[3];
		int pos;
		// the %n check prevents matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &pos) &&
				!strcmp(ext, "mca") && pos == strlen(ent->d_name))
		{
			// TODO: add support for .mcr files

			printf("Rendering region %d/%d at %d, %d\n", rc, count, rx, rz);

			// FIXME: path/filename joining needs to be more flexible
			char path[255];
			sprintf(path, "%s/%s", worlddir, ent->d_name);

			unsigned char* regionimage = render_region_blockmap(path, colours, alpha);
			for (int bz = 0; bz < REGION_BLOCK_WIDTH; bz++)
			{
				// copy a line of pixel data from the region image to the world image
				int offset = ((rz - rzmin) * REGION_BLOCK_WIDTH + bz) * w
						+ (rx - rxmin) * REGION_BLOCK_WIDTH;
				memcpy(&worldimage[offset * CHANNELS],
						&regionimage[bz * REGION_BLOCK_WIDTH * CHANNELS],
						REGION_BLOCK_WIDTH * CHANNELS);
			}
			free(regionimage);
			rc++;
		}
	}
	closedir(dir);

	printf("Encoding image...\n");
	lodepng_encode32_file(imagefile, worldimage, w, h);
	free(worldimage);
}
