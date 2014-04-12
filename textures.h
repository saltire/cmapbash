#ifndef TEXTURES_H
#define TEXTURES_H


#include "image.h"


// isometric pixel dimensions
#define ISO_BLOCK_WIDTH 4
#define ISO_BLOCK_TOP_HEIGHT 1
#define ISO_BLOCK_DEPTH 3
#define ISO_BLOCK_HEIGHT (ISO_BLOCK_TOP_HEIGHT + ISO_BLOCK_DEPTH)
#define ISO_BLOCK_AREA (ISO_BLOCK_WIDTH * ISO_BLOCK_HEIGHT)


typedef enum {
	BLANK,
	BLOCK,
	HILIGHT,
	SHADOW,
	COLOURS
} colourcodes;


typedef struct blocktype {
	unsigned char mask;
	struct subtype {
		unsigned char colour[CHANNELS];
		unsigned char shapeid;
	} subtypes[16];
} blocktype;


typedef struct shape {
	char is_solid;
	char has_hilight;
	char has_shadow;
	unsigned char pixels[ISO_BLOCK_AREA];
} shape;


typedef struct textures {
	int blockcount;
	blocktype* blocktypes;
	shape* shapes;
} textures;


textures* read_textures(const char* texturefile, const char* shapefile);
void free_textures(textures* tex);

const unsigned char* get_block_colour(const textures* tex,
		const unsigned char blockid, const unsigned char dataval);
const shape get_block_shape(const textures* tex,
		const unsigned char blockid, const unsigned char dataval);

void set_colour_brightness(unsigned char* pixel, float brightness, float ambience);
void adjust_colour_brightness(unsigned char* pixel, float mod);
void combine_alpha(unsigned char* top, unsigned char* bottom, int down);


#endif
