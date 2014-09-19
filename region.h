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


#include "chunk.h"
#include "textures.h"


void get_region_margins(const char* regionfile, int* margins, const char rotate);
void get_region_iso_margins(const char* regionfile, int* margins, const char rotate);

void render_tiny_region_map(image* image, const int rpx, const int rpy, const char* regionfile,
		const options opts);
void render_region_map(image* image, const int rpx, const int rpy, const char* regionfile,
		char* nfiles[4], const textures* tex, const options opts);

void save_region_map(const char* regionfile, const char* imagefile, const options opts);
