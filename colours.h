#define BLOCK_TYPES 176
#define CHANNELS 4
#define ALPHA CHANNELS - 1


typedef struct colour {
	unsigned char mask;
	unsigned char types[16 * CHANNELS];
} colour;


colour* read_colours(const char* filename);
