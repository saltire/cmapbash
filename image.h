#ifndef IMAGE_H
#define IMAGE_H


#define CHANNELS 4
#define ALPHA (CHANNELS - 1)

typedef struct image {
	unsigned int width, height;
	unsigned char* data;
} image;


image create_image(const unsigned int width, const unsigned int height);
void save_image(const image image, const char* outfile);
void free_image(image image);


#endif
