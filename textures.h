#ifndef TEXTURES_H
#define TEXTURES_H


#include "image.h"


// isometric pixel dimensions
#define ISO_BLOCK_WIDTH 4
#define ISO_BLOCK_TOP_HEIGHT 1
#define ISO_BLOCK_DEPTH 3
#define ISO_BLOCK_HEIGHT (ISO_BLOCK_TOP_HEIGHT + ISO_BLOCK_DEPTH)
#define ISO_BLOCK_AREA (ISO_BLOCK_WIDTH * ISO_BLOCK_HEIGHT)


typedef struct blocktype {
	unsigned char mask;
	struct subtype {
		unsigned char colour[CHANNELS];
		unsigned char shapeid;
	} subtypes[16];
} blocktype;


typedef struct textures {
	int blockcount;
	blocktype* blocktypes;
	unsigned char* shapes;
} textures;


textures* read_textures(const char* texturefile, const char* shapefile);
void free_textures(textures* tex);

const unsigned char* get_block_colour(const textures* tex,
		const unsigned char blockid, const unsigned char dataval);
const unsigned char get_block_shapeid(const textures* tex,
		const unsigned char blockid, const unsigned char dataval);

void set_colour_brightness(unsigned char* pixel, float brightness, float ambience);
void adjust_colour_brightness(unsigned char* pixel, float mod);
void combine_alpha(unsigned char* top, unsigned char* bottom, int down);


#endif
