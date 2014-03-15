#include <errno.h>
#include <stdlib.h>

#include "lodepng.h"

#include "colours.h"
#include "isoregion.h"


unsigned char* render_iso_region_blockmap(const char* regionfile, const colour* colours,
		const char night)
{
	FILE* region = fopen(regionfile, "r");
	if (region == NULL)
	{
		printf("Error %d reading region file: %s\n", errno, regionfile);
	}

	unsigned int offset[REGION_CHUNK_AREA];
	for (int c = 0; c < REGION_CHUNK_AREA; c++)
	{
		unsigned char buffer[4];
		fread(buffer, 1, 4, region);
		offset[c] = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
	}

	printf("Rendering isometric image, %d x %d\n", ISO_REGION_WIDTH, ISO_REGION_HEIGHT);
	unsigned char* regionimage = (unsigned char*)calloc(
			ISO_REGION_WIDTH * ISO_REGION_HEIGHT * CHANNELS, sizeof(char));

	for (int cz = 0; cz < REGION_CHUNK_WIDTH; cz++)
	{
		for (int cx = REGION_CHUNK_WIDTH - 1; cx >= 0; cx--)
		{
			int c = cz * REGION_CHUNK_WIDTH + cx;
			if (offset[c] == 0) continue;

			//printf("Reading chunk %d (%d, %d) from offset %d (at %#x).\n", c, cx, cz,
			//		offset[c], offset[c] * SECTOR_LENGTH);
			fseek(region, offset[c] * SECTOR_LENGTH, SEEK_SET);
			unsigned char buffer[4];
			fread(buffer, 1, 4, region);
			int length = (buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3]) - 1;

			fseek(region, 1, SEEK_CUR);
			//printf("Reading %d bytes at %#lx.\n", length, ftell(region));
			char* cdata = (char*)malloc(length);
			fread(cdata, length, 1, region);
			nbt_node* chunk = nbt_parse_compressed(cdata, length);
			if (errno != NBT_OK)
			{
				printf("Parsing error: %d\n", errno);
				return 0;
			}
			free(cdata);

			unsigned char* chunkimage = render_iso_chunk_blockmap(chunk, colours, night);
			nbt_free(chunk);

			int cpx = (cx + cz) * ISO_CHUNK_WIDTH / 2;
			int cpy = (REGION_CHUNK_WIDTH - cx + cz - 1) * CHUNK_BLOCK_WIDTH * ISO_BLOCK_STEP;
			printf("Writing chunk %d,%d to pixel %d,%d\n", cx, cz, cpx, cpy);

			for (int py = 0; py < ISO_CHUNK_HEIGHT; py++)
			{
				for (int px = 0; px < ISO_CHUNK_WIDTH; px++)
				{
					combine_alpha(&chunkimage[(py * ISO_CHUNK_WIDTH + px) * CHANNELS],
							&regionimage[((cpy + py) * ISO_REGION_WIDTH + cpx + px) * CHANNELS], 1);
				}
			}
			free(chunkimage);
		}
	}

	fclose(region);
	return regionimage;
}


void save_iso_region_blockmap(const char* regionfile, const char* imagefile, const colour* colours,
		const char night)
{
	unsigned char* regionimage = render_iso_region_blockmap(regionfile, colours, night);
	printf("Saving image to %s...\n", imagefile);
	lodepng_encode32_file(imagefile, regionimage, ISO_REGION_WIDTH, ISO_REGION_HEIGHT);
	free(regionimage);
}
