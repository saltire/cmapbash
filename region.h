#include "chunk.h"


#define REGION_CHUNK_WIDTH 32
#define REGION_BLOCK_WIDTH REGION_CHUNK_WIDTH * CHUNK_BLOCK_WIDTH
#define REGION_CHUNK_AREA REGION_CHUNK_WIDTH * REGION_CHUNK_WIDTH
#define REGION_BLOCK_AREA REGION_CHUNK_AREA * CHUNK_BLOCK_AREA
#define SECTOR_LENGTH 4096


unsigned char* render_region_blockmap(const char* regionfile, const colour* colours,
		const char alpha);
void save_region_blockmap(const char* regionfile, const char* imagefile, const colour* colours,
		const char alpha);
