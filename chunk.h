#ifndef CHUNK_H
#define CHUNK_H


#include "nbt.h"

#include "image.h"
#include "textures.h"


#define CHUNK_BLOCK_LENGTH 16
#define CHUNK_SECTION_HEIGHT 16
#define SECTION_BLOCK_HEIGHT 16
#define CHUNK_BLOCK_AREA CHUNK_BLOCK_LENGTH * CHUNK_BLOCK_LENGTH
#define CHUNK_BLOCK_HEIGHT SECTION_BLOCK_HEIGHT * CHUNK_SECTION_HEIGHT
#define SECTION_BLOCK_VOLUME SECTION_BLOCK_HEIGHT * CHUNK_BLOCK_AREA
#define CHUNK_BLOCK_VOLUME CHUNK_BLOCK_HEIGHT * CHUNK_BLOCK_AREA
#define ISO_BLOCK_STEP 1
#define ISO_CHUNK_WIDTH (CHUNK_BLOCK_LENGTH * ISO_BLOCK_WIDTH)
#define ISO_CHUNK_SURFACE_HEIGHT ((CHUNK_BLOCK_LENGTH * 2 - 1) * ISO_BLOCK_STEP)
#define ISO_CHUNK_DEPTH (ISO_BLOCK_HEIGHT * CHUNK_BLOCK_HEIGHT - ISO_BLOCK_STEP)
#define ISO_CHUNK_HEIGHT (ISO_CHUNK_SURFACE_HEIGHT + ISO_CHUNK_DEPTH)


void get_chunk_blockdata(nbt_node* chunk, unsigned char* blocks, unsigned char* data,
		unsigned char* blight);

int get_rotated_index(const int x, const int z, const int length, const unsigned char rotate);

image render_chunk_blockmap(nbt_node* chunk, const texture* textures, const char night,
		const char rotate);
image render_chunk_iso_blockmap(nbt_node* chunk, const texture* textures, const char night,
		const char rotate);

void save_chunk_blockmap(nbt_node* chunk, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate);

unsigned char* get_chunk_heightmap(nbt_node* chunk);

image render_chunk_heightmap(nbt_node* chunk);

void save_chunk_heightmap(nbt_node* chunk, const char* imagefile);


#endif
