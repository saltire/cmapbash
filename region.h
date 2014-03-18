#include "chunk.h"


#define REGION_CHUNK_WIDTH 32
#define REGION_BLOCK_WIDTH REGION_CHUNK_WIDTH * CHUNK_BLOCK_WIDTH
#define REGION_CHUNK_AREA REGION_CHUNK_WIDTH * REGION_CHUNK_WIDTH
#define REGION_BLOCK_AREA REGION_CHUNK_AREA * CHUNK_BLOCK_AREA
#define SECTOR_LENGTH 4096
#define ISO_REGION_WIDTH (ISO_CHUNK_WIDTH * REGION_CHUNK_WIDTH)
#define ISO_REGION_HEIGHT ((REGION_BLOCK_WIDTH - 1) * 2 * ISO_BLOCK_STEP \
	+ ISO_BLOCK_HEIGHT * CHUNK_BLOCK_HEIGHT)


void get_region_margins(const char* regionfile, int* margins);

image render_region_blockmap(const char* regionfile, const colour* colours,
		const char night);
image render_region_iso_blockmap(const char* regionfile, const colour* colours,
		const char night);

void save_region_blockmap(const char* regionfile, const char* imagefile, const colour* colours,
		const char night, const char isometric);
