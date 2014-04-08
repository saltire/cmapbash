#ifndef TEXTURES_H
#define TEXTURES_H


#include "image.h"


#define BLOCK_TYPES 176
#define SHAPE_COUNT 4

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
	blocktype blocktypes[BLOCK_TYPES];
	unsigned char shapes[SHAPE_COUNT][ISO_BLOCK_AREA];
} textures;


textures* read_textures(const char* texturefile);

const unsigned char* get_block_colour(const textures* textures,
		const unsigned char blockid, const unsigned char dataval);
const unsigned char get_block_shapeid(const textures* textures,
		const unsigned char blockid, const unsigned char dataval);

void set_colour_brightness(unsigned char* pixel, float brightness, float ambience);
void adjust_colour_brightness(unsigned char* pixel, float mod);
void combine_alpha(unsigned char* top, unsigned char* bottom, int down);


#endif
