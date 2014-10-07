/*
	cmapbash - a simple Minecraft map renderer written in C.
	© 2014 saltire sable, x@saltiresable.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <stdlib.h>


#include "lodepng.h"

#include "image.h"


image *create_image(const unsigned int width, const unsigned int height)
{
	image *img = (image*)malloc(sizeof(image));
	img->width = width;
	img->height = height;
	img->data = (unsigned char*)calloc(width * height * CHANNELS, sizeof(char));
	return img;
}


void save_image(const image *img, const char *imgfile)
{
	lodepng_encode32_file(imgfile, img->data, img->width, img->height);
}


void free_image(image *img)
{
	free(img->data);
	free(img);
}