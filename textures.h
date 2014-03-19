#ifndef TEXTURES_H
#define TEXTURES_H


#define ISO_BLOCK_WIDTH 4
#define ISO_BLOCK_HEIGHT 4
#define BLOCK_TYPES 176
#define CHANNELS 4
#define ALPHA CHANNELS - 1


typedef struct texture {
	unsigned char mask;
	struct blocktype {
		unsigned char colour[CHANNELS];
		unsigned char shape[ISO_BLOCK_WIDTH * ISO_BLOCK_HEIGHT];
	} types[16];
} texture;


texture* read_textures(const char* texturefile);

void adjust_colour_by_lum(unsigned char* pixel, unsigned char light);
void adjust_colour_by_height(unsigned char* pixel, int y);
void combine_alpha(unsigned char* top, unsigned char* bottom, int down);


#endif
