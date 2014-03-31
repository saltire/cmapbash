#include <stdlib.h>


#include "lodepng.h"

#include "image.h"


image create_image(const unsigned int width, const unsigned int height)
{
	image image = {
			.width = width,
			.height = height,
			.data = (unsigned char*)calloc(width * height * CHANNELS, sizeof(char))
	};
	return image;
}


void save_image(const image image, const char* outfile)
{
	lodepng_encode32_file(outfile, image.data, image.width, image.height);
}


void free_image(image image)
{
	free(image.data);
}
