#ifndef CHUNK_H
#define CHUNK_H


#include "nbt.h"

#include "colours.h"
#include "image.h"


#define CHUNK_BLOCK_WIDTH 16
#define CHUNK_SECTION_HEIGHT 16
#define SECTION_BLOCK_HEIGHT 16
#define CHUNK_BLOCK_AREA CHUNK_BLOCK_WIDTH * CHUNK_BLOCK_WIDTH
#define CHUNK_BLOCK_HEIGHT SECTION_BLOCK_HEIGHT * CHUNK_SECTION_HEIGHT
#define SECTION_BLOCK_VOLUME SECTION_BLOCK_HEIGHT * CHUNK_BLOCK_AREA
#define CHUNK_BLOCK_VOLUME CHUNK_BLOCK_HEIGHT * CHUNK_BLOCK_AREA
#define ISO_BLOCK_WIDTH 4
#define ISO_BLOCK_HEIGHT 4
#define ISO_BLOCK_STEP 1
#define ISO_CHUNK_WIDTH (CHUNK_BLOCK_WIDTH * ISO_BLOCK_WIDTH)
#define ISO_CHUNK_HEIGHT ((CHUNK_BLOCK_WIDTH - 1) * 2 * ISO_BLOCK_STEP \
	+ ISO_BLOCK_HEIGHT * CHUNK_BLOCK_HEIGHT)


void get_chunk_blockdata(nbt_node* chunk, unsigned char* blocks, unsigned char* data,
		unsigned char* blight);

image render_chunk_blockmap(nbt_node* chunk, const colour* colours, const char night);
image render_chunk_iso_blockmap(nbt_node* chunk, const colour* colours, const char night);

void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const colour* colours,
		const char night, const char isometric);

unsigned char* get_chunk_heightmap(nbt_node* chunk);

image render_chunk_heightmap(nbt_node* chunk);

void save_chunk_heightmap(nbt_node* chunk, const char* imagefile);


#endif
