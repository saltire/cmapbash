#include "lodepng.h"

#include <stdlib.h>


void render_greyscale(const char* filename, const unsigned char* values, unsigned w, unsigned h)
{
	unsigned char* image = (unsigned char*)malloc(w * h * 4);
	for (int i = 0; i < w * h; i++)
	{
		// colour values must be big-endian, so copy them manually
		image[i * 4] = values[i];
		image[i * 4 + 1] = values[i];
		image[i * 4 + 2] = values[i];
		image[i * 4 + 3] = 255;
	}
	lodepng_encode32_file("chunk.png", image, w, h);
	free(image);
}
