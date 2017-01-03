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


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "data.h"
#include "map.h"
#include "textures.h"


// get the width of empty space that would be on each edge of the rendered world map
static void get_world_margins(uint32_t *margins, const worldinfo *world, const bool isometric)
{
	// initialize margins to maximum, and decrease them as regions are found
	if (isometric)
	{
		margins[LEFT] = margins[RIGHT] = (world->rrxsize + world->rrzsize) * ISO_REGION_X_MARGIN;
		margins[TOP] = margins[BOTTOM] = (world->rrxsize + world->rrzsize) * ISO_REGION_Y_MARGIN
				- ISO_BLOCK_TOP_HEIGHT + ISO_CHUNK_DEPTH;
	}
	else
		for (uint8_t i = 0; i < 4; i++) margins[i] = REGION_BLOCK_LENGTH;

	for (uint32_t rrz = 0; rrz <= world->rrzmax; rrz++)
	{
		for (uint32_t rrx = 0; rrx <= world->rrxmax; rrx++)
		{
			// skip regions not on the edge of the map
			if (!isometric && rrx > 0 && rrx < world->rrxmax && rrz > 0 && rrz < world->rrzmax)
				continue;

			// get region, or skip if it doesn't exist
			region *reg = get_region_from_coords(world, rrx, rrz);
			if (reg == NULL) continue;

			uint32_t rmargins[4];
			get_region_margins(rmargins, reg, world->rotate, isometric);

			if (isometric)
			{
				// isometric offsets for this region
				uint32_t rro[] =
				{
					(rrx + rrz)                                 * ISO_REGION_Y_MARGIN, // top
					(world->rrxmax - rrx + rrz)                 * ISO_REGION_X_MARGIN, // right
					(world->rrxmax - rrx + world->rrzmax - rrz) * ISO_REGION_Y_MARGIN, // bottom
					(rrx + world->rrzmax - rrz)                 * ISO_REGION_X_MARGIN, // left
				};

				for (uint8_t i = 0; i < 4; i++)
				{
					// add region offset in pixels; if it's lower, update the final world margin
					uint32_t rmargin = rmargins[i] + rro[i];
					if (rmargin < margins[i]) margins[i] = rmargin;
				}
			}
			else
			{
				// use margins for the specific edge(s) that this region touches
				if (rrz == 0 && rmargins[TOP] < margins[TOP])
					margins[TOP] = rmargins[TOP];
				if (rrx == world->rrxmax && rmargins[RIGHT] < margins[RIGHT])
					margins[RIGHT] = rmargins[RIGHT];
				if (rrz == world->rrzmax && rmargins[BOTTOM] < margins[BOTTOM])
					margins[BOTTOM] = rmargins[BOTTOM];
				if (rrx == 0 && rmargins[LEFT] < margins[LEFT])
					margins[LEFT] = rmargins[LEFT];
			}
		}
	}
}


void render_world_map(image *img, int32_t wpx, int32_t wpy, const worldinfo *world,
		const options *opts)
{
	textures *tex = (opts->tiny ? NULL : read_textures(opts->texpath,
			opts->isometric ? opts->shapepath : NULL, opts->biomes ? opts->biomepath : NULL));

	uint32_t r = 0;
	// we need to render the regions in order from bottom to top for isometric view
	for (int32_t rrz = world->rrzmax; rrz >= 0; rrz--)
	{
		for (int32_t rrx = world->rrxmax; rrx >= 0; rrx--)
		{
			// get region, or skip if it doesn't exist
			region *reg = get_region_from_coords(world, rrx, rrz);
			if (reg == NULL) continue;

			r++;
			printf("Rendering region %d/%d (%d,%d)...\n", r, world->rcount, reg->x, reg->z);

			uint32_t rpx, rpy;

			if (opts->tiny)
			{
				rpx = rrx * REGION_CHUNK_LENGTH + wpx;
				rpy = rrz * REGION_CHUNK_LENGTH + wpy;
				render_tiny_region_map(img, rpx, rpy, reg, opts);
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

				// get rotated neighbouring regions
				region *nregions[4] =
				{
					get_region_from_coords(world, rrx, rrz - 1),
					get_region_from_coords(world, rrx + 1, rrz),
					get_region_from_coords(world, rrx, rrz + 1),
					get_region_from_coords(world, rrx - 1, rrz),
				};

				render_region_map(img, rpx, rpy, reg, nregions, tex, opts);
			}
		}
	}

	if (!opts->tiny) free_textures(tex);
}


image *create_world_map(char *worldpath, const options *opts)
{
	worldinfo *world = measure_world(worldpath,
		opts->rotate, opts->limits, opts->nether, opts->end);
	if (world == NULL) return NULL;

	uint32_t width, height, margins[4];
	get_world_margins(margins, world, opts->isometric);

	if (opts->tiny)
	{
		width  = world->rrxsize * REGION_CHUNK_LENGTH;
		height = world->rrzsize * REGION_CHUNK_LENGTH;
		// tiny map's scale is 1 pixel per chunk
		for (uint8_t i = 0; i < 4; i++) margins[i] /= CHUNK_BLOCK_LENGTH;
	}
	else if (opts->isometric)
	{
		width  = (world->rrxsize + world->rrzsize) * ISO_REGION_X_MARGIN;
		height = (world->rrxsize + world->rrzsize) * ISO_REGION_Y_MARGIN
				- ISO_BLOCK_TOP_HEIGHT + ISO_CHUNK_DEPTH;
		if (opts->ylimits != NULL)
		{
			margins[TOP] += (MAX_HEIGHT - opts->ylimits[1]) * ISO_BLOCK_DEPTH;
			margins[BOTTOM] += opts->ylimits[0] * ISO_BLOCK_DEPTH;
		}
	}
	else
	{
		width  = world->rrxsize * REGION_BLOCK_LENGTH;
		height = world->rrzsize * REGION_BLOCK_LENGTH;
	}
	width  -= (margins[LEFT] + margins[RIGHT]);
	height -= (margins[TOP] + margins[BOTTOM]);

	image *img = create_image(width, height);
	printf("Read %d regions. Image dimensions: %d x %d\n", world->rcount, width, height);

	clock_t start = clock();
	render_world_map(img, -margins[LEFT], -margins[TOP], world, opts);
	printf("Total render time: %f seconds\n", (double)(clock() - start) / CLOCKS_PER_SEC);

	free_world(world);

	return img;
}
