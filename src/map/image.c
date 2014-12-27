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


image *load_image(const char *imgfile)
{
	image *img = (image*)malloc(sizeof(image));
	lodepng_decode32_file(&img->data, &img->width, &img->height, imgfile);
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


image *scale_image_half(const image *img)
{
	image *simg = create_image((img->width + 1) / 2, (img->height + 1) / 2);

	for (unsigned int y = 0; y < img->height; y += 2)
	{
		for (unsigned int x = 0; x < img->width; x += 2)
		{
			unsigned int p = y * img->width + x;
			unsigned int sp = y * img->width / 4 + x / 2;

			for (int c = 0; c < CHANNELS; c++)
			{
				simg->data[sp * CHANNELS + c] = (
					img->data[p * CHANNELS + c] +
					(x + 1 == img->width ? 0 :
						img->data[(p + 1) * CHANNELS + c]) +
					(y + 1 == img->height ? 0 :
						img->data[(p + img->width) * CHANNELS + c]) +
					(x + 1 == img->width || y + 1 == img->height ? 0 :
						img->data[(p + img->width + 1) * CHANNELS + c])
				) / 4;
			}
		}
	}

	return simg;
}


void slice_image(const image *img, const unsigned int tilesize, const char *tiledir)
{
	unsigned int t = 0;
	unsigned int count = (
		(img->width + tilesize - 1) / tilesize *
		(img->height + tilesize - 1) / tilesize
	);

	for (unsigned int y = 0; y < img->width; y += tilesize)
	{
		for (unsigned int x = 0; x < img->height; x += tilesize)
		{
			t++;
			printf("Saving tile %d/%d...\n", t, count);

			image *tile = create_image(tilesize, tilesize);

			for (unsigned int ty = 0; ty < tilesize; ty++)
			{
				// if we're past the bottom of the main image, stop rendering this tile
				if (y + ty == img->height) break;

				unsigned char *row = &img->data[((y + ty) * img->width + x) * CHANNELS];
				unsigned char *trow = &tile->data[ty * tilesize * CHANNELS];

				// copy the lesser of the tile width, or the remainder of the image width
				unsigned int length = (x + tilesize > img->width) ? img->width - x : tilesize;

				memcpy(trow, row, length * CHANNELS);
			}

			char tilepath[255];
			sprintf(tilepath, "%s/%d.%d.png", tiledir, x / tilesize, y / tilesize);

			save_image(tile, tilepath);

			free_image(tile);
		}
	}
}
