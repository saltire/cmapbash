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


#include <stdlib.h>
#include <string.h>

#include "chunkmap.h"
#include "dims.h"
#include "image.h"
#include "regiondata.h"


void get_region_margins(unsigned int *rmargins, region *reg, const char rotate,
		const char isometric)
{
	for (int i = 0; i < 4; i++)
		rmargins[i] = isometric ? (rmargins[i] = i % 2 ? ISO_REGION_WIDTH : ISO_REGION_TOP_HEIGHT)
				: REGION_BLOCK_LENGTH;

	for (unsigned int cz = reg->climits[0]; cz <= reg->climits[2]; cz++)
	{
		for (unsigned int cx = reg->climits[3]; cx <= reg->climits[1]; cx++)
		{
			if (!reg->offsets[cz * REGION_CHUNK_LENGTH + cx]) continue;

			// block margins for this chunk
			unsigned int cm[4] =
			{
				MAX(cz * CHUNK_BLOCK_LENGTH, reg->blimits[0]),
				MAX_REGION_BLOCK - MIN(cx * CHUNK_BLOCK_LENGTH + MAX_CHUNK_BLOCK, reg->blimits[1]),
				MAX_REGION_BLOCK - MIN(cz * CHUNK_BLOCK_LENGTH + MAX_CHUNK_BLOCK, reg->blimits[2]),
				MAX(cx * CHUNK_BLOCK_LENGTH, reg->blimits[3]),
			};

			// rotated pixel margins for this chunk
			unsigned int rcm[4];
			for (int i = 0; i < 4; i++)
			{
				int m = (i + rotate) % 4;
				rcm[m] = isometric ? ((cm[i] + cm[(i + 3) % 4])
						* (m % 2 ? ISO_BLOCK_X_MARGIN : ISO_BLOCK_Y_MARGIN))
						: cm[i];

				// assign to rotated region margins if lower
				if (rcm[m] < rmargins[m]) rmargins[m] = rcm[m];
			}
		}
	}
}


void render_tiny_region_map(image *img, const int rpx, const int rpy, region *reg,
		const options *opts)
{
	open_region_file(reg);
	if (reg == NULL || reg->file == NULL) return;

	unsigned char colour[CHANNELS] = {255, 255, 255, 255};

	for (unsigned int rcz = 0; rcz < REGION_CHUNK_LENGTH; rcz++)
	{
		for (unsigned int rcx = 0; rcx < REGION_CHUNK_LENGTH; rcx++)
		{
			if (!chunk_exists(reg, rcx, rcz, opts->rotate)) continue;

			// colour this pixel
			int cpx = rpx + rcx;
			int cpy = rpy + rcz;
			unsigned char *pixel = &img->data[(cpy * img->width + cpx) * CHANNELS];
			memcpy(pixel, colour, CHANNELS);
		}
	}

	close_region_file(reg);
}


void render_region_map(image *img, const int rpx, const int rpy, region *reg,
		region *nregions[4], const textures *tex, const options *opts)
{
	open_region_file(reg);
	if (reg == NULL || reg->file == NULL) return;

	for (int i = 0; i < 4; i++) open_region_file(nregions[i]);

	nbt_node *chunk, *prev_chunk, *new_chunk, *nchunks[4];
	// use rotated chunk coordinates, since we need to draw them top to bottom for isometric
	for (unsigned int rcz = 0; rcz < REGION_CHUNK_LENGTH; rcz++)
	{
		for (unsigned int rcx = 0; rcx < REGION_CHUNK_LENGTH; rcx++)
		{
			// get the actual chunk from its rotated coordinates
			// use the "new" chunk saved by the previous iteration if possible
			chunk = rcx > 0 && prev_chunk != NULL ? new_chunk :
					read_chunk(reg, rcx, rcz, opts->rotate);
			if (chunk == NULL)
			{
				if (rcx > 0) nbt_free(prev_chunk);
				prev_chunk = NULL;
				continue;
			}

			// get neighbouring chunks, either from this region or a neighbouring one
			nchunks[0] = rcz > 0 ? read_chunk(reg, rcx, rcz - 1, opts->rotate) :
					read_chunk(nregions[0], rcx, MAX_REGION_CHUNK, opts->rotate);

			nchunks[1] = rcx < MAX_REGION_CHUNK ? read_chunk(reg, rcx + 1, rcz, opts->rotate) :
					read_chunk(nregions[1], 0, rcz, opts->rotate);

			nchunks[2] = rcz < MAX_REGION_CHUNK ? read_chunk(reg, rcx, rcz + 1, opts->rotate) :
					read_chunk(nregions[2], rcx, 0, opts->rotate);

			nchunks[3] = rcx > 0 ? prev_chunk :
					read_chunk(nregions[3], MAX_REGION_CHUNK, rcz, opts->rotate);

			// if this chunk is on the edge of region chunk limits, pass it the block limits
			int *cblimits;
			if (opts->limits == NULL) cblimits = NULL;
			else
			{
				unsigned int co = get_chunk_offset(rcx, rcz, opts->rotate);
				unsigned int cx = co % REGION_CHUNK_LENGTH;
				unsigned int cz = co / REGION_CHUNK_LENGTH;

				int edge = 0;
				int cblimits_arr[4] = {0, MAX_CHUNK_BLOCK, MAX_CHUNK_BLOCK, 0};

				for (int i = 0; i < 4; i++)
					if (reg->climits[i] == (i % 2 ? cx : cz))
					{
						edge = 1;
						cblimits_arr[i] = reg->blimits[i] & MAX_CHUNK_BLOCK;
					}
				cblimits = edge ? cblimits_arr : NULL;
			}

			// render chunk image onto region image
			int cpx, cpy;
			if (opts->isometric)
			{
				// translate orthographic to isometric coordinates
				cpx = rpx + (rcx + MAX_REGION_CHUNK - rcz) * ISO_CHUNK_X_MARGIN;
				cpy = rpy + (rcx + rcz) * ISO_CHUNK_Y_MARGIN;
			}
			else
			{
				cpx = rpx + rcx * CHUNK_BLOCK_LENGTH;
				cpy = rpy + rcz * CHUNK_BLOCK_LENGTH;
			}
			render_chunk_map(img, cpx, cpy, chunk, nchunks, cblimits, tex, opts);

			// free chunks, or save them for the next iteration if we're not at the end of a row
			nbt_free(nchunks[0]);
			nbt_free(nchunks[2]);
			nbt_free(nchunks[3]);
			if (rcx == MAX_REGION_CHUNK)
			{
				nbt_free(chunk);
				nbt_free(nchunks[1]);
			}
			else
			{
				prev_chunk = chunk;
				new_chunk = nchunks[1];
			}
		}
	}

	close_region_file(reg);
	for (int i = 0; i < 4; i++) close_region_file(nregions[i]);
}
