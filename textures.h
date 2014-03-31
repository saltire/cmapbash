#ifndef TEXTURES_H
#define TEXTURES_H


#define BLOCK_TYPES 176
#define MAX_LIGHT 16
#define CHANNELS 4
#define ALPHA CHANNELS - 1

// configurable render options
#define HEIGHT_MIDPOINT 0.3 // midpoint of height shading gradient
#define HEIGHT_CONTRAST 0.7 // amount of contrast for height shading
#define NIGHT_AMBIENCE 0.2 // base light level for night renders

// isometric pixel dimensions
#define ISO_BLOCK_WIDTH 4
#define ISO_BLOCK_TOP_HEIGHT 1
#define ISO_BLOCK_DEPTH 3
#define ISO_BLOCK_HEIGHT (ISO_BLOCK_TOP_HEIGHT + ISO_BLOCK_DEPTH)


typedef struct texture {
	unsigned char mask;
	struct blocktype {
		unsigned char colour[CHANNELS];
		unsigned char shape[ISO_BLOCK_WIDTH * ISO_BLOCK_HEIGHT];
	} types[16];
} texture;


texture* read_textures(const char* texturefile);

void set_colour_brightness(unsigned char* pixel, float brightness, float ambience);
void adjust_colour_brightness(unsigned char* pixel, float mod);
void combine_alpha(unsigned char* top, unsigned char* bottom, int down);


#endif
