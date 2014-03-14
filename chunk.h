#include "nbt.h"

#include "lodepng.h"

#include "colours.h"


#define CHUNK_BLOCK_WIDTH 16
#define CHUNK_SECTION_HEIGHT 16
#define SECTION_BLOCK_HEIGHT 16
#define CHUNK_BLOCK_AREA CHUNK_BLOCK_WIDTH * CHUNK_BLOCK_WIDTH
#define CHUNK_BLOCK_HEIGHT SECTION_BLOCK_HEIGHT * CHUNK_SECTION_HEIGHT
#define SECTION_BLOCK_VOLUME SECTION_BLOCK_HEIGHT * CHUNK_BLOCK_AREA
#define CHUNK_BLOCK_VOLUME CHUNK_BLOCK_HEIGHT * CHUNK_BLOCK_AREA


unsigned char* render_chunk_blockmap(nbt_node* chunk, const colour* colours, const char night);
void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const colour* colours,
		const char night);

unsigned char* get_chunk_heightmap(nbt_node* chunk);
unsigned char* render_chunk_heightmap(nbt_node* chunk);
void save_chunk_heightmap(nbt_node* chunk, const char* imagefile);
