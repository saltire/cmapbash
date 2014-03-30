#ifndef IMAGE_H
#define IMAGE_H


#include "lodepng.h"


typedef struct image {
	unsigned int width, height;
	unsigned char* data;
} image;


#define SAVE_IMAGE(image, outfile) \
	lodepng_encode32_file((outfile), (image).data, (image).width, (image).height)

#define FREE_IMAGE(image) free(image.data)


#endif
