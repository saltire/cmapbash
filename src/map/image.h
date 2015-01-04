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


#include <stdint.h>


#define CHANNELS 4
#define ALPHA (CHANNELS - 1)


typedef struct image {
	uint32_t width, height; // pixel dimensions of the image
	uint8_t* data;          // pointer to an RGBA pixel buffer for the image
} image;


/* create an image struct
 *   width, height: pixel dimensions of the image
 */
image *create_image(const uint32_t width, const uint32_t height);

/* load an image struct from a PNG file
 *   imgpath: path to the image file
 */
image *load_image(const char *imgpath);

/* save an image struct to a PNG file
 *   img:     pointer to the image struct
 *   imgpath: path to the output file
 */
void save_image(const image *img, const char *imgpath);

/* free the memory used for an image struct
 *   img: pointer to the image struct
 */
void free_image(image *img);

/* create an image struct with a copy of the given image scaled to 50% of its dimensions
 *   img: pointer to the source image struct
 */
image *scale_image_half(const image *img);

/* slice an image into square tiles of a certain size and save them to individual files
 *   img:      pointer to the source image struct
 *   tilesize: length and width of each tile
 *   tilepath: path of the directory to save tile images to
 */
void slice_image(const image *img, const uint32_t tilesize, const char *tiledir);


#endif
