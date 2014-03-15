#include "isochunk.h"
#include "region.h"


#define ISO_REGION_WIDTH (ISO_CHUNK_WIDTH * REGION_CHUNK_WIDTH)
#define ISO_REGION_HEIGHT ((REGION_BLOCK_WIDTH - 1) * 2 * ISO_BLOCK_STEP \
	+ ISO_BLOCK_HEIGHT * CHUNK_BLOCK_HEIGHT)


unsigned char* render_iso_region_blockmap(const char* regionfile, const colour* colours,
		const char night);
void save_iso_region_blockmap(const char* regionfile, const char* imagefile, const colour* colours,
		const char night);
