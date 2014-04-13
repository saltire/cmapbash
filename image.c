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
