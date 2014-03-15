#ifndef COLOURS_H
#define COLOURS_H


#define BLOCK_TYPES 176
#define CHANNELS 4
#define ALPHA CHANNELS - 1


typedef struct colour {
	unsigned char mask;
	unsigned char types[16 * CHANNELS];
} colour;


colour* read_colours(const char* filename);

void adjust_colour_by_lum(unsigned char* pixel, unsigned char light);
void adjust_colour_by_height(unsigned char* pixel, int y);
void combine_alpha(unsigned char* top, unsigned char* bottom, int down);


#endif
