#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lodepng.h"

#include "colours.h"
#include "region.h"


void save_world_blockmap(const char* worlddir, const char* imagefile, const colour* colours,
		const char night)
{
	DIR* dir = opendir(worlddir);
	if (dir == NULL)
	{
		printf("Error %d reading region directory: %s\n", errno, worlddir);
		return;
	}

	// list directory to find the number of region files, and image dimensions
	int rcount = 0;
	int rx, rz, rxmin, rxmax, rzmin, rzmax;
	struct dirent* ent;
	char ext[4]; // three-char extension + null char
	int length; // check filename length to prevent matching filenames with trailing characters
	while ((ent = readdir(dir)) != NULL)
	{
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			if (!rcount)
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
			rcount++;
		}
	}
	if (rcount == 0)
	{
		printf("No regions found in directory: %s\n", worlddir);
		return;
	}

	char path[255];

	int margins[4];
	int rmargins[4];
	for (int i = 0; i < 4; i++)
	{
		margins[i] = REGION_BLOCK_WIDTH;
	}

	rewinddir(dir);
	while ((ent = readdir(dir)) != NULL)
	{
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			// check margins if this region is on the edge of the map
			if (rx == rxmin || rx == rxmax || rz == rzmin || rz == rzmax)
			{
				sprintf(path, "%s/%s", worlddir, ent->d_name);
				get_region_margins(path, rmargins);
				//printf("Margins for region %d, %d: N %d, E %d, S %d, W %d\n",
				//		rx, rz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

				// use margins for the specific edge(s) that this region touches
				if (rz == rzmin) // north
				{
					margins[0] = rmargins[0] < margins[0] ? rmargins[0] : margins[0];
				}
				if (rx == rxmax) // east
				{
					margins[1] = rmargins[1] < margins[1] ? rmargins[1] : margins[1];
				}
				if (rz == rzmax) // south
				{
					margins[2] = rmargins[2] < margins[2] ? rmargins[2] : margins[2];
				}
				if (rx == rxmin) // west
				{
					margins[3] = rmargins[3] < margins[3] ? rmargins[3] : margins[3];
				}
			}
		}
	}
	//printf("Margins: N %d, E %d, S %d, W %d\n", margins[0], margins[1], margins[2], margins[3]);

	int w = (rxmax - rxmin + 1) * REGION_BLOCK_WIDTH - margins[1] - margins[3];
	int h = (rzmax - rzmin + 1) * REGION_BLOCK_WIDTH - margins[0] - margins[2];
	printf("Read %d regions. Image dimensions: %d x %d\n", rcount, w, h);
	unsigned char* worldimage = (unsigned char*)malloc(w * h * CHANNELS);

	// create an array of region files and
	rewinddir(dir);
	int rc = 0;
	while ((ent = readdir(dir)) != NULL)
	{
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			// TODO: add support for .mcr files

			rc++;
			printf("Rendering region %d/%d at %d, %d\n", rc, rcount, rx, rz);

			// FIXME: path/filename joining needs to be more flexible
			sprintf(path, "%s/%s", worlddir, ent->d_name);

			unsigned char* regionimage = render_region_blockmap(path, colours, night);

			int rxoffset = (rx == rxmin ? margins[3] : 0);
			int rwidth = REGION_BLOCK_WIDTH - rxoffset - (rx == rxmax ? margins[1] : 0);

			int rzoffset = (rz == rzmin ? margins[0] : 0);
			int rheight = REGION_BLOCK_WIDTH - rzoffset - (rz == rzmax ? margins[2] : 0);

			for (int z = 0; z < rheight; z++)
			{
				// copy a line of pixel data from the region image to the world image
				int woffset = ((rz - rzmin) * REGION_BLOCK_WIDTH - margins[0] + rzoffset + z) * w
						+ (rx - rxmin) * REGION_BLOCK_WIDTH - margins[3] + rxoffset;
				int roffset = (rzoffset + z) * REGION_BLOCK_WIDTH + rxoffset;
				memcpy(&worldimage[woffset * CHANNELS],
						&regionimage[roffset * CHANNELS],
						rwidth * CHANNELS);
			}
			free(regionimage);
		}
	}
	closedir(dir);

	printf("Encoding image to %s ...\n", imagefile);
	lodepng_encode32_file(imagefile, worldimage, w, h);
	free(worldimage);
}
