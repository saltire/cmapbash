#include "chunk.h"


#define ISO_BLOCK_WIDTH 4
#define ISO_BLOCK_HEIGHT 4
#define ISO_BLOCK_STEP 1
#define ISO_IMAGE_WIDTH (CHUNK_BLOCK_WIDTH * ISO_BLOCK_WIDTH)
#define ISO_IMAGE_HEIGHT ((CHUNK_BLOCK_WIDTH - 1) * 2 * ISO_BLOCK_STEP \
	+ ISO_BLOCK_HEIGHT * CHUNK_BLOCK_HEIGHT)


unsigned char* render_iso_chunk_blockmap(nbt_node* chunk, const colour* colours, const char night);
void save_iso_chunk_blockmap(nbt_node* chunk, const char* imagefile, const colour* colours,
		const char night);
