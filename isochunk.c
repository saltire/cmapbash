#include <stdlib.h>

#include "isochunk.h"


unsigned char* render_iso_chunk_blockmap(nbt_node* chunk, const colour* colours, const char night)
{
	unsigned char* blocks = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* data = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	unsigned char* blight = NULL;
	if (night)
	{
		blight = (unsigned char*)calloc(CHUNK_BLOCK_VOLUME, sizeof(char));
	}
	get_chunk_blockdata(chunk, blocks, data, blight);

	printf("Rendering isometric image, %d x %d\n", ISO_IMAGE_WIDTH, ISO_IMAGE_HEIGHT);
	unsigned char* image = (unsigned char*)calloc(
			ISO_IMAGE_WIDTH * ISO_IMAGE_HEIGHT * CHANNELS, sizeof(char));

	for (int by = 0; by < CHUNK_BLOCK_HEIGHT; by++)
	{
		for (int bz = 0; bz < CHUNK_BLOCK_WIDTH; bz++)
		{
			for (int bx = CHUNK_BLOCK_WIDTH - 1; bx >= 0; bx--)
			{
				int b = by * CHUNK_BLOCK_AREA + bz * CHUNK_BLOCK_WIDTH + bx;
				unsigned char blockid = blocks[b];
				unsigned char type = data[b] % colours[blockid].mask;
				unsigned char* colour = (unsigned char*)malloc(CHANNELS);
				memcpy(colour, &colours[blockid].types[type * CHANNELS], CHANNELS);
				adjust_colour_by_height(colour, by);
				if (blight != NULL)
				{
					adjust_colour_by_lum(colour, blight[b]);
				}

				int px = (bx + bz) * ISO_BLOCK_WIDTH / 2;
				int py = (CHUNK_BLOCK_WIDTH - bx + bz - 1) * ISO_BLOCK_STEP
						+ (CHUNK_BLOCK_HEIGHT - by - 1) * ISO_BLOCK_HEIGHT;
				//printf("Block %d,%d,%d rendering at pixel %d,%d\n", bx, bz, by, px, py);
				for (int y = py; y < py + ISO_BLOCK_HEIGHT; y++)
				{
					for (int x = px; x < px + ISO_BLOCK_WIDTH; x++)
					{
						combine_alpha(colour, &image[(y * ISO_IMAGE_WIDTH + x) * CHANNELS], 1);
					}
				}
			}
		}
	}
	free(blocks);
	free(data);
	free(blight);
	return image;
}


void save_iso_chunk_blockmap(nbt_node* chunk, const char* imagefile, const colour* colours,
		const char night)
{
	unsigned char* chunkimage = render_iso_chunk_blockmap(chunk, colours, night);
	printf("Saving to %s\n", imagefile);
	lodepng_encode32_file(imagefile, chunkimage, ISO_IMAGE_WIDTH, ISO_IMAGE_HEIGHT);
	free(chunkimage);
}
