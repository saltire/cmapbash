#include "chunk.h"
#include "textures.h"


// world dimensions
#define REGION_CHUNK_LENGTH 32
#define REGION_BLOCK_LENGTH (REGION_CHUNK_LENGTH * CHUNK_BLOCK_LENGTH)
#define REGION_CHUNK_AREA (REGION_CHUNK_LENGTH * REGION_CHUNK_LENGTH)
#define REGION_BLOCK_AREA (REGION_CHUNK_AREA * CHUNK_BLOCK_AREA)

// isometric pixel dimensions
#define ISO_REGION_WIDTH (ISO_CHUNK_WIDTH * REGION_CHUNK_LENGTH)
#define ISO_REGION_TOP_HEIGHT ((REGION_BLOCK_LENGTH * 2 - 1) * ISO_BLOCK_TOP_HEIGHT)
#define ISO_REGION_HEIGHT (ISO_REGION_TOP_HEIGHT + ISO_CHUNK_DEPTH)
#define ISO_REGION_X_MARGIN (ISO_REGION_WIDTH / 2)
#define ISO_REGION_Y_MARGIN (REGION_BLOCK_LENGTH * ISO_BLOCK_TOP_HEIGHT)


void get_region_margins(const char* regionfile, int* margins, const char rotate);
void get_region_iso_margins(const char* regionfile, int* margins, const char rotate);

void render_region_map(image* image, const int rpx, const int rpy,
		const char* regionfile, char* nfiles[4], const textures* tex,
		const char night, const char isometric, const char rotate);
void save_region_map(const char* regionfile, const char* imagefile, const textures* tex,
		const char night, const char isometric, const char rotate);
