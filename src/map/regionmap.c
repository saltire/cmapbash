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
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "image.h"
#include "map.h"
#include "textures.h"


void get_region_margins(uint32_t *rmargins, region *reg, const uint8_t rotate,
		const bool isometric)
{
	for (int i = 0; i < 4; i++)
		rmargins[i] = isometric ? (rmargins[i] = i % 2 ? ISO_REGION_WIDTH : ISO_REGION_TOP_HEIGHT)
				: REGION_BLOCK_LENGTH;

	uint16_t rblimits[4] = {0, MAX_REGION_BLOCK, MAX_REGION_BLOCK, 0};
	uint8_t rclimits[4] = {0, MAX_REGION_CHUNK, MAX_REGION_CHUNK, 0};

	if (reg->blimits != NULL)
		for (int i = 0; i < 4; i++)
		{
			rblimits[i] = reg->blimits[i];
			rclimits[i] = reg->blimits[i] >> CHUNK_BLOCK_BITS;
		}

	for (uint8_t cz = rclimits[NORTH]; cz <= rclimits[SOUTH]; cz++)
	{
		for (uint8_t cx = rclimits[WEST]; cx <= rclimits[EAST]; cx++)
		{
			if (!reg->offsets[cz * REGION_CHUNK_LENGTH + cx]) continue;

			// block margins for this chunk
			uint32_t cm[4] =
			{
				                   MAX(rblimits[NORTH], cz * CHUNK_BLOCK_LENGTH),
				MAX_REGION_BLOCK - MIN(rblimits[EAST],  cx * CHUNK_BLOCK_LENGTH + MAX_CHUNK_BLOCK),
				MAX_REGION_BLOCK - MIN(rblimits[SOUTH], cz * CHUNK_BLOCK_LENGTH + MAX_CHUNK_BLOCK),
				                   MAX(rblimits[WEST],  cx * CHUNK_BLOCK_LENGTH),
			};

			// rotated pixel margins for this chunk
			uint32_t rcm[4];
			for (uint8_t i = 0; i < 4; i++)
			{
				uint8_t m = (i + rotate) % 4;

				rcm[m] = isometric ? (cm[i] + cm[(i + 3) % 4])
						* (m % 2 ? ISO_BLOCK_X_MARGIN : ISO_BLOCK_Y_MARGIN)
						: cm[i];

				// assign to rotated region margins if lower
				if (rcm[m] < rmargins[m]) rmargins[m] = rcm[m];
			}
		}
	}
}


void render_tiny_region_map(image *img, const int32_t rpx, const int32_t rpy, region *reg,
		const options *opts)
{
	open_region_file(reg);
	if (reg == NULL || reg->file == NULL) return;

	uint8_t colour[CHANNELS] = {255, 255, 255, 255};

	for (uint8_t rcz = 0; rcz < REGION_CHUNK_LENGTH; rcz++)
	{
		for (uint8_t rcx = 0; rcx < REGION_CHUNK_LENGTH; rcx++)
		{
			if (!chunk_exists(reg, rcx, rcz, opts->rotate)) continue;

			// colour this pixel
			uint8_t *pixel = &img->data[((rpy + rcz) * img->width + rpx + rcx) * CHANNELS];
			memcpy(pixel, colour, CHANNELS);
		}
	}

	close_region_file(reg);
}


void render_region_map(image *img, const int32_t rpx, const int32_t rpy, region *reg,
		region *nregions[4], const textures *tex, const options *opts)
{
	open_region_file(reg);
	if (reg == NULL || reg->file == NULL) return;

	for (uint8_t i = 0; i < 4; i++) open_region_file(nregions[i]);

	chunk_data *chunk, *prev_chunk, *new_chunk, *nchunks[4];
	chunk_flags flags = {
		1,
		1,
		opts->night,
		opts->isometric && !opts->night && opts->shadows,
		opts->biomes
	};
	chunk_flags nflags = {
		1,
		opts->isometric,
		opts->isometric && opts->night,
		opts->isometric && !opts->night && opts->shadows,
		0
	};

	// use rotated chunk coordinates, since we need to draw them top to bottom for isometric
	for (uint8_t rcz = 0; rcz < REGION_CHUNK_LENGTH; rcz++)
	{
		for (uint8_t rcx = 0; rcx < REGION_CHUNK_LENGTH; rcx++)
		{
			// get the actual chunk from its rotated coordinates
			// use the "new" chunk saved by the previous iteration if possible
			chunk = rcx > 0 && prev_chunk != NULL ? new_chunk :
					read_chunk(reg, rcx, rcz, opts->rotate, &flags, opts->ylimits);
			if (chunk == NULL)
			{
				if (rcx > 0) free_chunk(prev_chunk);
				prev_chunk = NULL;
				continue;
			}

			// get neighbouring chunks, either from this region or a neighbouring one
			nchunks[0] = rcz > 0 ?
					read_chunk(reg, rcx, rcz - 1, opts->rotate, &flags, opts->ylimits) :
					read_chunk(nregions[0], rcx, MAX_REGION_CHUNK, opts->rotate, &nflags,
							opts->ylimits);

			nchunks[1] = rcx < MAX_REGION_CHUNK ?
					read_chunk(reg, rcx + 1, rcz, opts->rotate, &flags, opts->ylimits) :
					read_chunk(nregions[1], 0, rcz, opts->rotate, &nflags, opts->ylimits);

			nchunks[2] = rcz < MAX_REGION_CHUNK ?
					read_chunk(reg, rcx, rcz + 1, opts->rotate, &flags, opts->ylimits) :
					read_chunk(nregions[2], rcx, 0, opts->rotate, &nflags, opts->ylimits);

			nchunks[3] = rcx > 0 ? prev_chunk :
					read_chunk(nregions[3], MAX_REGION_CHUNK, rcz, opts->rotate, &nflags,
							opts->ylimits);

			for (uint8_t i = 0; i < 4; i++)
			{
				if (nchunks[i] == NULL)
				{
					chunk->nbids[i] = NULL;
					chunk->nbdata[i] = NULL;
					chunk->nblight[i] = NULL;
					chunk->nslight[i] = NULL;
				}
				else {
					chunk->nbids[i] = nchunks[i]->bids;
					chunk->nbdata[i] = nchunks[i]->bdata;
					chunk->nblight[i] = nchunks[i]->blight;
					chunk->nslight[i] = nchunks[i]->slight;
				}
			}

			// render chunk image onto region image
			uint32_t cpx, cpy;
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

			// loop through rotated chunk's blocks
			for (uint16_t rbz = 0; rbz <= MAX_CHUNK_BLOCK; rbz++)
				for (uint16_t rbx = 0; rbx <= MAX_CHUNK_BLOCK; rbx++)
					if (opts->isometric)
						render_iso_column(img, cpx, cpy, tex, chunk, rbx, rbz, opts);
					else
						render_ortho_column(img, cpx, cpy, tex, chunk, rbx, rbz, opts);

			// free chunks, or save them for the next iteration if we're not at the end of a row
			free_chunk(nchunks[0]);
			free_chunk(nchunks[2]);
			free_chunk(nchunks[3]);
			if (rcx == MAX_REGION_CHUNK)
			{
				free_chunk(chunk);
				free_chunk(nchunks[1]);
			}
			else
			{
				prev_chunk = chunk;
				new_chunk = nchunks[1];
			}
		}
	}

	close_region_file(reg);
	for (uint8_t i = 0; i < 4; i++) close_region_file(nregions[i]);
}
