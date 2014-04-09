#ifndef CHUNK_H
#define CHUNK_H


#include "nbt.h"

#include "image.h"
#include "textures.h"


// world dimensions
#define CHUNK_BLOCK_LENGTH 16
#define CHUNK_SECTION_HEIGHT 16
#define SECTION_BLOCK_HEIGHT 16
#define CHUNK_BLOCK_AREA (CHUNK_BLOCK_LENGTH * CHUNK_BLOCK_LENGTH)
#define CHUNK_BLOCK_HEIGHT (SECTION_BLOCK_HEIGHT * CHUNK_SECTION_HEIGHT)
#define SECTION_BLOCK_VOLUME (SECTION_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)
#define CHUNK_BLOCK_VOLUME (CHUNK_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)

// isometric pixel dimensions
#define ISO_CHUNK_WIDTH (CHUNK_BLOCK_LENGTH * ISO_BLOCK_WIDTH)
#define ISO_CHUNK_TOP_HEIGHT ((CHUNK_BLOCK_LENGTH * 2 - 1) * ISO_BLOCK_TOP_HEIGHT)
#define ISO_CHUNK_DEPTH (ISO_BLOCK_DEPTH * CHUNK_BLOCK_HEIGHT)
#define ISO_CHUNK_HEIGHT (ISO_CHUNK_TOP_HEIGHT + ISO_CHUNK_DEPTH)
#define ISO_CHUNK_X_MARGIN (ISO_CHUNK_WIDTH / 2)
#define ISO_CHUNK_Y_MARGIN (CHUNK_BLOCK_LENGTH * ISO_BLOCK_TOP_HEIGHT)


void render_chunk_map(image* image, const int cpx, const int cpy,
		nbt_node* chunk, nbt_node* nchunks[4], const textures* textures,
		const char night, const char isometric, const char rotate);
void save_chunk_map(nbt_node* chunk, const char* imagefile, const textures* textures,
		const char night, const char isometric, const char rotate);


#endif
