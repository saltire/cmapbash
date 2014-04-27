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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dims.h"
#include "image.h"
#include "textures.h"


#define LINE_BUFFER 60


typedef enum {
	BLOCKID,
	MASK,
	SUBTYPE,
	RED1,
	GREEN1,
	BLUE1,
	ALPHA1,
	RED2,
	GREEN2,
	BLUE2,
	ALPHA2,
	NOISE,
	SHAPE,
	COLUMN_COUNT
} columns;


textures* read_textures(const char* texturefile, const char* shapefile)
{
	textures* tex = (textures*)calloc(1, sizeof(textures));

	char line[LINE_BUFFER];

	// shape file
	FILE* scsv = fopen(shapefile, "r");
	if (scsv == NULL) printf("Error %d reading shape file: %s\n", errno, shapefile);

	// count shapes and allocate array
	int c, scount = 0;
	while ((c = getc(scsv)) != EOF) if (c == '\n') scount++;
	shape shapes[scount];

	// read shape pixel maps
	int s = 0;
	fseek(scsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, scsv))
	{
		shapes[s].is_solid = 1;
		for (int i = 0; i < ISO_BLOCK_AREA; i++)
		{
			// convert ascii value to numeric value
			unsigned char pcolour = line[i] - '0';
			shapes[s].pixels[i] = pcolour;

			// set flags
			if (shapes[s].pixels[i] == BLANK) shapes[s].is_solid = 0;
			shapes[s].has[pcolour] = 1;
		}
		s++;
	}
	fclose(scsv);

	// colour file
	FILE* tcsv = fopen(texturefile, "r");
	if (tcsv == NULL) printf("Error %d reading texture file: %s\n", errno, texturefile);

	// find highest block id and allocate blocktypes array
	unsigned char blockid = -1;
	while (fgets(line, LINE_BUFFER, tcsv)) blockid = (char)strtol(strtok(line, ","), NULL, 0);
	tex->max_blockid = blockid;
	tex->blockids = (blockID*)calloc(tex->max_blockid + 1, sizeof(blockID));

	// read block textures
	fseek(tcsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, tcsv))
	{
		unsigned char row[COLUMN_COUNT];
		char* pos = line;
		for (int i = 0; i < COLUMN_COUNT; i++)
		{
			// read the length of the next value
			size_t len = (pos < line + LINE_BUFFER) ? strcspn(pos, ",") : 0;

			// read the value string and parse it as an integer
			char value[LINE_BUFFER];
			strncpy(value, pos, len);
			*(value + len) = '\0';
			row[i] = strcmp(value, "") ? (char)strtol(value, NULL, 0) : 0;

			// advance the pointer, unless we are at the end of the buffer
			if (pos < line + LINE_BUFFER) pos += len + 1;
		}

		// subtype mask is specified on subtype 0 of each block type
		if (row[SUBTYPE] == 0) tex->blockids[row[BLOCKID]].mask = row[MASK];

		// copy values for this block type
		blocktype* btype = &tex->blockids[row[BLOCKID]].subtypes[row[SUBTYPE]];
		btype->id = row[BLOCKID];
		btype->subtype = row[SUBTYPE];
		memcpy(&btype->colour1, &row[RED1], CHANNELS);
		memcpy(&btype->colour2, &row[RED2], CHANNELS);
		btype->is_opaque = (row[ALPHA1] == 255 && (row[ALPHA2] == 255 || row[ALPHA2] == 0));
		btype->shape = shapes[row[SHAPE]];
	}
	fclose(tcsv);

	return tex;
}


void free_textures(textures* tex)
{
	free(tex->blockids);
	free(tex);
}


const blocktype* get_block_type(const textures* tex,
		const unsigned char blockid, const unsigned char dataval)
{
	return &tex->blockids[blockid].subtypes[dataval % tex->blockids[blockid].mask];
}


void set_colour_brightness(unsigned char* pixel, float brightness, float ambience)
{
	if (pixel[ALPHA] == 0) return;

	for (int c = 0; c < ALPHA; c++)
		// darken pixel to ambient light only, then add brightness
		pixel[c] *= (ambience + (1 - ambience) * brightness);
}


void adjust_colour_brightness(unsigned char* pixel, float mod)
{
	if (pixel[ALPHA] == 0) return;

	for (int c = 0; c < ALPHA; c++)
		// room to adjust, multiplied by modifier %, gives us the amount to adjust
		pixel[c] += (mod < 0 ? pixel[c] : 255 - pixel[c]) * mod;
}


void combine_alpha(unsigned char* top, unsigned char* bottom, int down)
{
	if (top[ALPHA] == 255 || bottom[ALPHA] == 0)
	{
		if (down && top[ALPHA] > 0) memcpy(bottom, top, CHANNELS);
		return;
	}
	else if (top[ALPHA] == 0)
	{
		if (!down && bottom[ALPHA] > 0) memcpy(top, bottom, CHANNELS);
		return;
	}

	float bmod = (float)(255 - top[ALPHA]) / 255;
	unsigned char alpha = top[ALPHA] + bottom[ALPHA] * bmod;
	unsigned char* target = down ? bottom : top;
	for (int ch = 0; ch < ALPHA; ch++)
		target[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
	target[ALPHA] = alpha;
}
