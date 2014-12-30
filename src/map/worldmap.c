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


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "data.h"
#include "map.h"
#include "textures.h"


static void get_world_margins(unsigned int *margins, const worldinfo *world, const char isometric)
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

	for (unsigned int rrz = 0; rrz <= world->rrzmax; rrz++)
	{
		for (unsigned int rrx = 0; rrx <= world->rrxmax; rrx++)
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
					unsigned int rmargin = rmargins[i] + rro[i];
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


void render_world_map(image *image, int wpx, int wpy, const worldinfo *world, const options *opts)
{
	textures *tex = (opts->tiny ? NULL : read_textures(opts->texpath,
			opts->isometric ? opts->shapepath : NULL, opts->biomes ? opts->biomepath : NULL));

	int r = 0;
	// we need to render the regions in order from top to bottom for isometric view
	for (int rrz = 0; rrz <= world->rrzmax; rrz++)
	{
		for (int rrx = 0; rrx <= world->rrxmax; rrx++)
		{
			// get region, or skip if it doesn't exist
			region *reg = get_region_from_coords(world, rrx, rrz);
			if (reg == NULL) continue;

			r++;
			printf("Rendering region %d/%d (%d,%d)...\n", r, world->rcount, reg->x, reg->z);

			int rpx, rpy;

			if (opts->tiny)
			{
				rpx = rrx * REGION_CHUNK_LENGTH + wpx;
				rpy = rrz * REGION_CHUNK_LENGTH + wpy;
				render_tiny_region_map(image, rpx, rpy, reg, opts);
			}
			else
			{
				if (opts->isometric)
				{
					// translate orthographic region coordinates to isometric pixel coordinates
					rpx = (rrx + world->rrzmax - rrz) * ISO_REGION_X_MARGIN + wpx;
					rpy = (rrx + rrz)                 * ISO_REGION_Y_MARGIN + wpy;
				}
				else
				{
					rpx = rrx * REGION_BLOCK_LENGTH + wpx;
					rpy = rrz * REGION_BLOCK_LENGTH + wpy;
				}

				// get rotated neighbouring region paths
				region *nregions[4] =
				{
					get_region_from_coords(world, rrx, rrz - 1),
					get_region_from_coords(world, rrx + 1, rrz),
					get_region_from_coords(world, rrx, rrz + 1),
					get_region_from_coords(world, rrx - 1, rrz),
				};

				render_region_map(image, rpx, rpy, reg, nregions, tex, opts);
			}
		}
	}

	if (!opts->tiny) free_textures(tex);
}


void save_world_map(char *worldpath, const char *imgfile, const options *opts)
{
	worldinfo *world = measure_world(worldpath, opts->rotate, opts->limits);
	if (!world->rcount) return;

	unsigned int width, height, margins[4];
	get_world_margins(margins, world, opts->isometric);

	if (opts->tiny)
	{
		width  = world->rrxsize * REGION_CHUNK_LENGTH;
		height = world->rrzsize * REGION_CHUNK_LENGTH;
		// tiny map's scale is 1 pixel per chunk
		for (int i = 0; i < 4; i++) margins[i] /= CHUNK_BLOCK_LENGTH;
	}
	else if (opts->isometric)
	{
		width  = (world->rrxsize + world->rrzsize) * ISO_REGION_X_MARGIN;
		height = (world->rrxsize + world->rrzsize) * ISO_REGION_Y_MARGIN
				- ISO_BLOCK_TOP_HEIGHT + ISO_CHUNK_DEPTH;
		if (opts->ylimits != NULL)
		{
			margins[0] += (MAX_HEIGHT - opts->ylimits[1]) * ISO_BLOCK_DEPTH;
			margins[2] += opts->ylimits[0] * ISO_BLOCK_DEPTH;
		}
	}
	else
	{
		width  = world->rrxsize * REGION_BLOCK_LENGTH;
		height = world->rrzsize * REGION_BLOCK_LENGTH;
	}
	width  -= (margins[1] + margins[3]);
	height -= (margins[0] + margins[2]);

	image *img = create_image(width, height);
	printf("Read %d regions. Image dimensions: %d x %d\n", world->rcount, width, height);

	clock_t start = clock();
	render_world_map(img, -margins[3], -margins[0], world, opts);
	printf("Total render time: %f seconds\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	free_world(world);

	printf("Saving image to %s ...\n", imgfile);
	start = clock();
	save_image(img, imgfile);
	printf("Total save time: %f seconds\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	free_image(img);
}
