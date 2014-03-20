#include "chunk.h"
#include "textures.h"


#define REGION_CHUNK_LENGTH 32
#define REGION_BLOCK_LENGTH REGION_CHUNK_LENGTH * CHUNK_BLOCK_LENGTH
#define REGION_CHUNK_AREA REGION_CHUNK_LENGTH * REGION_CHUNK_LENGTH
#define REGION_BLOCK_AREA REGION_CHUNK_AREA * CHUNK_BLOCK_AREA
#define SECTOR_LENGTH 4096
#define ISO_REGION_WIDTH (ISO_CHUNK_WIDTH * REGION_CHUNK_LENGTH)
#define ISO_REGION_SURFACE_HEIGHT ((REGION_BLOCK_LENGTH * 2 - 1) * ISO_BLOCK_STEP)
#define ISO_REGION_HEIGHT (ISO_REGION_SURFACE_HEIGHT + ISO_CHUNK_DEPTH)


void get_region_margins(const char* regionfile, int* margins, const char rotate);
void get_region_iso_margins(const char* regionfile, int* margins, const char rotate);

image render_region_blockmap(const char* regionfile, const texture* textures, const char night,
		const char rotate);
image render_region_iso_blockmap(const char* regionfile, const texture* textures, const char night,
		const char rotate);

void save_region_blockmap(const char* regionfile, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate);
