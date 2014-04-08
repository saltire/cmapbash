#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "textures.h"


#define COLUMNS 9


unsigned char shapes[SHAPE_COUNT][ISO_BLOCK_AREA] = {
		// 0: solid
		{
				1,1,1,1,
				1,1,1,1,
				1,1,1,1,
				1,1,1,1
		},

		// 1: flat
		{
				0,0,0,0,
				0,0,0,0,
				0,0,0,0,
				1,1,1,1
		},

		// 2: small square
		{
				0,0,0,0,
				0,0,0,0,
				0,1,1,0,
				0,1,1,0
		},

		// 3: grass
		{
				0,0,0,0,
				0,0,1,0,
				1,0,1,1,
				0,1,1,0
		}
};


textures* read_textures(const char* texturefile)
{
	FILE* csv = fopen(texturefile, "r");
	if (csv == NULL)
	{
		printf("Error %d reading colour file: %s\n", errno, texturefile);
	}

	textures* tex = (textures*)calloc(1, sizeof(textures));
	for (int s = 0; s < SHAPE_COUNT; s++)
		for (int p = 0; p < ISO_BLOCK_AREA; p++)
			tex->shapes[s][p] = shapes[s][p];

	char* line = (char*)malloc(60);
	while (fgets(line, 60, csv))
	{
		unsigned char row[COLUMNS];
		for (int i = 0; i < COLUMNS; i++)
		{
			char* value = strtok(i == 0 ? line : NULL, ",");
			row[i] = (value == NULL ? 0 : (char)strtol(value, NULL, 0));
		}
		unsigned char blockid = row[0];
		unsigned char subtype = row[2];

		// subtype mask is specified on subtype 0 of each block type
		if (subtype == 0) tex->blocktypes[blockid].mask = row[1];
		memcpy(&tex->blocktypes[blockid].subtypes[subtype].colour, &row[3], CHANNELS);
		tex->blocktypes[blockid].subtypes[subtype].shapeid = row[8];
	}
	free(line);

	fclose(csv);
	return tex;
}


const unsigned char* get_block_colour(const textures* textures,
		const unsigned char blockid, const unsigned char dataval)
{
	unsigned char type = dataval % textures->blocktypes[blockid].mask;
	return textures->blocktypes[blockid].subtypes[type].colour;
}


const unsigned char get_block_shapeid(const textures* textures,
		const unsigned char blockid, const unsigned char dataval)
{
	unsigned char type = dataval % textures->blocktypes[blockid].mask;
	return textures->blocktypes[blockid].subtypes[type].shapeid;
}


void set_colour_brightness(unsigned char* pixel, float brightness, float ambience)
{
	if (pixel[ALPHA] == 0) return;

	for (int c = 0; c < ALPHA; c++)
	{
		pixel[c] *= (ambience + (1 - ambience) * brightness);
	}
}


void adjust_colour_brightness(unsigned char* pixel, float mod)
{
	if (pixel[ALPHA] == 0) return;

	for (int c = 0; c < ALPHA; c++)
	{
		unsigned char limit = mod < 0 ? pixel[c] : 255 - pixel[c]; // room to adjust
		pixel[c] += limit * mod;
	}
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
	{
		target[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
	}
	target[ALPHA] = alpha;
}
