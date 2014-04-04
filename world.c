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

	world.rxsize = world.rxmax - world.rxmin + 1;
	world.rzsize = world.rzmax - world.rzmin + 1;

	// be sure to free this pointer later
	world.regionmap = (unsigned char*)calloc(world.rxsize * world.rzsize, sizeof(char));

	rewinddir(dir);
	while ((ent = readdir(dir)) != NULL)
	{
		// use %n to check filename length to prevent matching filenames with trailing characters
		if (sscanf(ent->d_name, "r.%d.%d.%3s%n", &rx, &rz, ext, &length) &&
				!strcmp(ext, "mca") && length == strlen(ent->d_name))
		{
			// get rotated world-relative region coords from absolute region coords
			int rwx, rwz;
			switch(rotate) {
			case 0:
				rwx = rx - world.rxmin;
				rwz = rz - world.rzmin;
				break;
			case 1:
				rwx = world.rzsize - (rz - world.rzmin) - 1;
				rwz = rx - world.rxmin;
				break;
			case 2:
				rwx = world.rxsize - (rx - world.rxmin) - 1;
				rwz = world.rzsize - (rz - world.rzmin) - 1;
				break;
			case 3:
				rwx = rz - world.rzmin;
				rwz = world.rxsize - (rx - world.rxmin) - 1;
				break;
			}
			world.regionmap[(rz - world.rzmin) * world.rxsize + rx - world.rxmin] = 1;
		}
	}
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

	// initialize margins to maximum, and decrease them as regions are parsed
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

	// world margins measured in pixels
	// start at maximum and decrease as regions are found
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


image render_world_blockmap(world world, const texture* textures, const char night,
		const char rotate)
{
	int margins[4];
	get_world_margins(world, margins, rotate);

	int rwxsize = rotate % 2 ? world.rzsize : world.rxsize;
	int rwzsize = rotate % 2 ? world.rxsize : world.rzsize;

	image wimage = create_image(
			rwxsize * REGION_BLOCK_LENGTH - margins[1] - margins[3],
			rwzsize * REGION_BLOCK_LENGTH - margins[0] - margins[2]);
	printf("Read %d regions. Image dimensions: %d x %d\n",
			world.rcount, wimage.width, wimage.height);

	int r = 0;
	for (int rwz = 0; rwz < rwzsize; rwz++)
	{
		for (int rwx = 0; rwx < rwxsize; rwx++)
		{
			char path[255] = "";
			get_path_from_rel_coords(path, world, rwx, rwz, rotate);
			if (!strcmp(path, "")) continue;

			image rimage = render_region_blockmap(path, textures, night, rotate);
			if (rimage.data == NULL)
			{
				free_image(rimage);
				continue;
			}

			int rxo = (rwx == 0 ? margins[3] : 0);
			int rpx = rwx * REGION_BLOCK_LENGTH - margins[3] + rxo;
			int rw = REGION_BLOCK_LENGTH - rxo - (rwx == rwxsize - 1 ? margins[1] : 0);

			int ryo = (rwz == 0 ? margins[0] : 0);
			int rpy = rwz * REGION_BLOCK_LENGTH - margins[0] + ryo;
			int rh = REGION_BLOCK_LENGTH - ryo - (rwz == rwzsize - 1 ? margins[2] : 0);

			r++;
			printf("Rendering region %d/%d at pos %d,%d pixel %d,%d\n",
					r, world.rcount, rwx, rwz, rpx, rpy);
			for (int y = 0; y < rh; y++)
			{
				// copy a line of pixel data from the region image to the world image
				memcpy(&wimage.data[((rpy + y) * wimage.width + rpx) * CHANNELS],
						&rimage.data[((ryo + y) * REGION_BLOCK_LENGTH + rxo) * CHANNELS],
						rw * CHANNELS);
			}
			free_image(rimage);
		}
	}

	return wimage;
}


image render_world_iso_blockmap(world world, const texture* textures, const char night,
		const char rotate)
{
	int margins[4];
	get_world_iso_margins(world, margins, rotate);

	int rwxsize = rotate % 2 ? world.rzsize : world.rxsize;
	int rwzsize = rotate % 2 ? world.rxsize : world.rzsize;

	image wimage = create_image(
			(rwxsize + rwzsize) * ISO_REGION_X_MARGIN - margins[1] - margins[3],
			((rwxsize + rwzsize) * ISO_REGION_Y_MARGIN - ISO_BLOCK_TOP_HEIGHT)
						+ ISO_CHUNK_DEPTH - margins[0] - margins[2]);
	printf("Read %d regions. Image dimensions: %d x %d\n",
			world.rcount, wimage.width, wimage.height);

	int r = 0;
	// we need to render the regions in a certain order
	for (int rwz = 0; rwz < rwzsize; rwz++)
	{
		for (int rwx = 0; rwx < rwxsize; rwx++)
		{
			char path[255] = "";
			get_path_from_rel_coords(path, world, rwx, rwz, rotate);
			if (!strcmp(path, "")) continue;

			// translate orthographic region coordinates to isometric pixel coordinates
			int rpx = (rwx + rwzsize - 1 - rwz) * ISO_REGION_X_MARGIN - margins[3];
			int rpy = (rwx + rwz) * ISO_REGION_Y_MARGIN - margins[0];

			// find pixel offsets where regions will be cropped at top or left edge
			int rxo = rpx < 0 ? -rpx : 0;
			int ryo = rpy < 0 ? -rpy : 0;

			// find region pixel width and height
			int rw = rpx + ISO_REGION_WIDTH > wimage.width ?
					wimage.width - rpx - rxo : ISO_REGION_WIDTH - rxo;
			int rh = rpy + ISO_REGION_HEIGHT > wimage.height ?
					wimage.height - rpy - ryo : ISO_REGION_HEIGHT - ryo;

			rpx += rxo;
			rpy += ryo;

			r++;
			printf("Rendering region %d/%d at pos %d,%d pixel %d,%d\n",
					r, world.rcount, rwx, rwz, rpx, rpy);

			image rimage = render_region_iso_blockmap(path, textures, night, rotate);
			if (rimage.data == NULL)
			{
				free_image(rimage);
				continue;
			}

			for (int py = 0; py < rh; py++)
			{
				for (int px = 0; px < rw; px++)
				{
					combine_alpha(
							&rimage.data[((ryo + py) * ISO_REGION_WIDTH + rxo + px) * CHANNELS],
							&wimage.data[((rpy + py) * wimage.width + rpx + px) * CHANNELS], 1);
				}
			}
			free_image(rimage);
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
	free_world(world);

	printf("Saving image to %s ...\n", imagefile);
	save_image(wimage, imagefile);
	free_image(wimage);
}
