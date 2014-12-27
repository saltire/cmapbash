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


#ifndef IMAGE_H
#define IMAGE_H


#define CHANNELS 4
#define ALPHA (CHANNELS - 1)

typedef struct image {
	unsigned int width, height;
	unsigned char* data;
} image;


image *create_image(const unsigned int width, const unsigned int height);

image *load_image(const char *imgfile);

void save_image(const image *img, const char *imgfile);

void free_image(image *img);

image *scale_image_half(const image *img);

void slice_image(const image *img, const unsigned int tilesize, const char *tiledir);


#endif
