#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "region.h"
#include "textures.h"
#include "world.h"


static world measure_world(const char* worlddir, const char rotate)
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

	world.rxsize = (world.rxmax - world.rxmin + 1);
	world.rzsize = (world.rzmax - world.rzmin + 1);

	// be sure to free this pointer later
	world.regions = (int*)malloc(world.rcount * sizeof(int) * 4);

	rewinddir(dir);
	int r = 0;
	int rwx, rwz;
	while ((ent = readdir(dir)) != NULL)
	{
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			// region coordinates relative to the rotated world
			switch(rotate) {
			case 0:
				rwx = rx - world.rxmin;
				rwz = rz - world.rzmin;
				break;
			case 1:
				rwx = world.rxsize - (rz - world.rzmin) - 1;
				rwz = rx - world.rxmin;
				break;
			case 2:
				rwx = world.rxsize - (rx - world.rxmin) - 1;
				rwz = world.rzsize - (rz - world.rzmin) - 1;
				break;
			case 3:
				rwx = rz - world.rzmin;
				rwz = world.rzsize - (rx - world.rxmin) - 1;
				break;
			}

			world.regions[r * 4] = rx;
			world.regions[r * 4 + 1] = rz;
			world.regions[r * 4 + 2] = rwx;
			world.regions[r * 4 + 3] = rwz;
			r++;
		}
	}
	closedir(dir);

	return world;
}


static void get_world_margins(world world, int* margins, const char rotate)
{
	// initialize margins to maximum, and decrease them as regions are parsed
	for (int i = 0; i < 4; i++)
	{
		margins[i] = REGION_BLOCK_LENGTH;
	}

	for (int r = 0; r < world.rcount; r++)
	{
		int rx = world.regions[r * 4];
		int rz = world.regions[r * 4 + 1];
		int rwx = world.regions[r * 4 + 2];
		int rwz = world.regions[r * 4 + 3];

		// check margins if this region is on the edge of the map
		if (rwx == 0 || rwx == world.rxsize - 1 || rwz == 0 || rwz == world.rzsize - 1)
		{
			char path[255];
			// FIXME: path/filename joining needs to be more flexible
			sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);

			int rmargins[4];
			get_region_margins(path, rmargins, rotate);
			//printf("Margins for region %d, %d: top %d, right %d, bottom %d, left %d\n",
			//		rx, rz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

			// use margins for the specific edge(s) that this region touches
			if (rwz == 0 && rmargins[0] < margins[0]) margins[0] = rmargins[0];
			if (rwx == world.rxsize - 1 && rmargins[1] < margins[1]) margins[1] = rmargins[1];
			if (rwz == world.rzsize - 1 && rmargins[2] < margins[2]) margins[2] = rmargins[2];
			if (rwx == 0 && rmargins[3] < margins[3]) margins[3] = rmargins[3];
		}
	}
	//printf("Margins: top %d, right %d, bottom %d, left %d\n",
	//		margins[0], margins[1], margins[2], margins[3]);
}


static void get_world_iso_margins(world world, int* margins, const char rotate)
{
	// initialize margins to maximum, and decrease them as regions are parsed

	int wrm[4]; // world margins measured in regions
	for (int i = 0; i < 4; i++)
	{
		wrm[i] = world.rxsize + world.rzsize;
	}
	// world margins measured in pixels
	margins[0] = ((world.rxsize + world.rzsize) * REGION_BLOCK_LENGTH - 1) * ISO_BLOCK_STEP;
	margins[1] = (world.rxsize + world.rzsize + 1) * ISO_REGION_WIDTH / 2;
	margins[2] = ((world.rxsize + world.rzsize) * REGION_BLOCK_LENGTH - 1) * ISO_BLOCK_STEP;
	margins[3] = (world.rxsize + world.rzsize + 1) * ISO_REGION_WIDTH / 2;

	for (int r = 0; r < world.rcount; r++)
	{
		int rx = world.regions[r * 4];
		int rz = world.regions[r * 4 + 1];
		int rwx = world.regions[r * 4 + 2];
		int rwz = world.regions[r * 4 + 3];

		// world margins for this region, measured in regions
		int wrtop = world.rxsize - rwx - 1 + rwz;
		int wrright = world.rxsize - rwx - 1 + world.rzsize - rwz - 1;
		int wrbottom = rwx + world.rzsize - rwz - 1;
		int wrleft = rwx + rwz;

		if (wrtop < wrm[0] || wrright < wrm[1] || wrbottom < wrm[2] || wrleft < wrm[3])
		{
			char path[255];
			// FIXME: path/filename joining needs to be more flexible
			sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);

			int rmargins[4];
			get_region_iso_margins(path, rmargins, rotate);
			//printf("Margins for region %d, %d: top %d, right %d, bottom %d, left %d\n",
			//		rx, rz, rmargins[0], rmargins[1], rmargins[2], rmargins[3]);

			if (wrtop <= wrm[0])
			{
				wrm[0] = wrtop;
				int top = wrtop * REGION_BLOCK_LENGTH * ISO_BLOCK_STEP + rmargins[0];
				if (top < margins[0]) margins[0] = top;
			}
			if (wrright <= wrm[1])
			{
				wrm[1] = wrright;
				int right = wrright * ISO_REGION_WIDTH / 2 + rmargins[1];
				if (right < margins[1]) margins[1] = right;
			}
			if (wrbottom <= wrm[2])
			{
				wrm[2] = wrbottom;
				int bottom = wrbottom * REGION_BLOCK_LENGTH * ISO_BLOCK_STEP + rmargins[2];
				if (bottom < margins[2]) margins[2] = bottom;
			}
			if (wrleft <= wrm[3])
			{
				wrm[3] = wrleft;
				int left = wrleft * ISO_REGION_WIDTH / 2 + rmargins[3];
				if (left < margins[3]) margins[3] = left;
			}
		}
	}
	//printf("World margins in regions: top %d, right %d, bottom %d, left %d\n",
	//		wrm[0], wrm[1], wrm[2], wrm[3]);
	//printf("World margins in pixels: top %d, right %d, bottom %d, left %d\n",
	//		margins[0], margins[1], margins[2], margins[3]);
}


image render_world_blockmap(world world, const texture* textures, const char night,
		const char rotate)
{
	int margins[4];
	get_world_margins(world, margins, rotate);

	image wimage;
	wimage.width = world.rxsize * REGION_BLOCK_LENGTH - margins[1] - margins[3];
	wimage.height = world.rzsize * REGION_BLOCK_LENGTH - margins[0] - margins[2];
	wimage.data = (unsigned char*)calloc(wimage.width * wimage.height * CHANNELS, sizeof(char));
	printf("Read %d regions. Image dimensions: %d x %d\n",
			world.rcount, wimage.width, wimage.height);

	for (int r = 0; r < world.rcount; r++)
	{
		int rx = world.regions[r * 4];
		int rz = world.regions[r * 4 + 1];
		int rwx = world.regions[r * 4 + 2];
		int rwz = world.regions[r * 4 + 3];

		char path[255];
		// FIXME: path/filename joining needs to be more flexible
		sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);
		image rimage = render_region_blockmap(path, textures, night, rotate);

		int rxo = (rwx == 0 ? margins[3] : 0);
		int rpx = rwx * REGION_BLOCK_LENGTH - margins[3] + rxo;
		int rw = REGION_BLOCK_LENGTH - rxo - (rwx == world.rxsize - 1 ? margins[1] : 0);

		int ryo = (rwz == 0 ? margins[0] : 0);
		int rpy = rwz * REGION_BLOCK_LENGTH - margins[0] + ryo;
		int rh = REGION_BLOCK_LENGTH - ryo - (rwz == world.rzsize - 1 ? margins[2] : 0);

		printf("Rendering region %d/%d (%d,%d) at pos %d,%d pixel %d,%d\n",
				r + 1, world.rcount, rx, rz, rwx, rwz, rpx, rpy);
		for (int y = 0; y < rh; y++)
		{
			// copy a line of pixel data from the region image to the world image
			memcpy(&wimage.data[((rpy + y) * wimage.width + rpx) * CHANNELS],
					&rimage.data[((ryo + y) * REGION_BLOCK_LENGTH + rxo) * CHANNELS],
					rw * CHANNELS);
		}
		free(rimage.data);
	}

	return wimage;
}


image render_world_iso_blockmap(world world, const texture* textures, const char night,
		const char rotate)
{
	int margins[4];
	get_world_iso_margins(world, margins, rotate);

	image wimage;
	wimage.width = (world.rxsize + world.rzsize) * ISO_REGION_WIDTH / 2 - margins[1] - margins[3];
	wimage.height = ((world.rxsize + world.rzsize) * REGION_BLOCK_LENGTH - 1) * ISO_BLOCK_STEP
			+ ISO_CHUNK_DEPTH - margins[0] - margins[2];
	wimage.data = (unsigned char*)calloc(wimage.width * wimage.height * CHANNELS, sizeof(char));
	printf("Read %d regions. Image dimensions: %d x %d\n",
			world.rcount, wimage.width, wimage.height);

	int rc = 0;
	int rx, rz;
	// we need to render the regions in a certain order
	for (int rwz = 0; rwz < world.rzsize; rwz++)
	{
		for (int rwx = world.rxsize - 1; rwx >= 0; rwx--)
		{
			// only read the region if we know it exists
			int exists = 0;
			for (int i = 0; i < world.rcount; i++)
			{
				if (world.regions[i * 4 + 2] == rwx && world.regions[i * 4 + 3] == rwz)
				{
					rx = world.regions[i * 4];
					rz = world.regions[i * 4 + 1];
					exists = 1;
					break;
				}
			}
			if (!exists) continue;

			int rpx = (rwx + rwz) * ISO_REGION_WIDTH / 2 - margins[3];
			int rxo = rpx < 0 ? -rpx : 0;
			int rw = ISO_REGION_WIDTH - rxo - (rpx + ISO_REGION_WIDTH > wimage.width
					? rpx + ISO_REGION_WIDTH - wimage.width : 0);
			rpx += rxo;

			int rpy = (world.rxsize - rwx + rwz - 1) * REGION_BLOCK_LENGTH * ISO_BLOCK_STEP
					- margins[0];
			int ryo = rpy < 0 ? -rpy : 0;
			int rh = ISO_REGION_HEIGHT - ryo - (rpy + ISO_REGION_HEIGHT > wimage.height
					? rpy + ISO_REGION_HEIGHT - wimage.height : 0);
			rpy += ryo;

			rc++;
			printf("Rendering region %d/%d (%d,%d) at pos %d,%d pixel %d,%d\n",
					rc, world.rcount, rx, rz, rwx, rwz, rpx, rpy);

			char path[255];
			sprintf(path, "%s/r.%d.%d.mca", world.dir, rx, rz);
			image rimage = render_region_iso_blockmap(path, textures, night, rotate);

			for (int py = 0; py < rh; py++)
			{
				for (int px = 0; px < rw; px++)
				{
					combine_alpha(
							&rimage.data[((ryo + py) * ISO_REGION_WIDTH + rxo + px) * CHANNELS],
							&wimage.data[((rpy + py) * wimage.width + rpx + px) * CHANNELS], 1);
				}
			}
			free(rimage.data);
		}
	}

	return wimage;
}


void save_world_blockmap(const char* worlddir, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate)
{
	world world = measure_world(worlddir, rotate);
	if (!world.rcount)
	{
		printf("No regions found in directory: %s\n", worlddir);
		return;
	}

	image wimage = isometric
			? render_world_iso_blockmap(world, textures, night, rotate)
			: render_world_blockmap(world, textures, night, rotate);
	free(world.regions);

	printf("Saving image to %s ...\n", imagefile);
	SAVE_IMAGE(wimage, imagefile);
	free(wimage.data);
}
