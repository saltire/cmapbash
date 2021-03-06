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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "map.h"
#include "textures.h"


#define LINE_BUFFER 100


typedef enum
{
	BLOCKID,
	SUBTYPE_MASK,
	SUBTYPE,
	RED1,
	GREEN1,
	BLUE1,
	ALPHA1,
	BIOME_COLOUR1,
	RED2,
	GREEN2,
	BLUE2,
	ALPHA2,
	BIOME_COLOUR2,
	SHAPE_N,
	SHAPE_E,
	SHAPE_S,
	SHAPE_W,
	TEXCOL_COUNT
}
texcols;

typedef enum
{
	BIOMEID,
	TEMPERATURE,
	RAINFALL,
	FOLIAGE_R,
	FOLIAGE_G,
	FOLIAGE_B,
	GRASS_R,
	GRASS_G,
	GRASS_B,
	BIOMECOL_COUNT
}
biomecols;


// generate an array of shape structs from a CSV file
static void read_shapes(shape **shapes, const char *shapepath)
{
	FILE *scsv = fopen(shapepath, "r");
	if (scsv == NULL) printf("Error %d reading shape file: %s\n", errno, shapepath);
	char line[LINE_BUFFER];

	// count shapes and allocate array
	uint32_t c;
	uint8_t scount = 0;
	while ((c = fgetc(scsv)) != EOF) if (c == '\n') scount++;
	*shapes = (shape*)calloc(scount, sizeof(shape));

	// read shape pixel maps
	int s = 0;
	fseek(scsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, scsv))
	{
		for (uint8_t p = 0; p < ISO_BLOCK_AREA; p++)
		{
			// convert ascii value to numeric value
			uint8_t pcolour = line[p] - '0';
			// store colour code for this pixel
			(*shapes)[s].pixmap[p] = pcolour;
			// increment the count for this colour code
			(*shapes)[s].clrcount[pcolour] += 1;
		}
		s++;
	}
	fclose(scsv);
}


// generate an array of biome structs from a CSV file
static int read_biomes(biome **biomes, const char *biomepath)
{
	FILE *bcsv = fopen(biomepath, "r");
	if (bcsv == NULL) printf("Error %d reading biome file: %s\n", errno, biomepath);
	char line[LINE_BUFFER];

	// find highest biome id and allocate biomes array
	uint8_t max_biomeid = -1;
	while (fgets(line, LINE_BUFFER, bcsv))
		max_biomeid = (uint8_t)strtol(strtok(line, ","), NULL, 0);
	*biomes = (biome*)calloc(max_biomeid + 1, sizeof(biome));

	// read biome colours
	fseek(bcsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, bcsv))
	{
		uint8_t id;
		char *pos = line;
		for (uint8_t i = 0; i < BIOMECOL_COUNT; i++)
		{
			// read the length of the next value
			size_t len = (pos < line + LINE_BUFFER) ? strcspn(pos, ",") : 0;
			// store the value in the buffer
			char value[LINE_BUFFER];
			strncpy(value, pos, len);
			*(value + len) = '\0';
			// advance the pointer, unless we are at the end of the buffer
			if (pos < line + LINE_BUFFER) pos += len + 1;

			// remember id
			if (i == BIOMEID)
			{
				id = (uint8_t)strtol(value, NULL, 0);
				(*biomes)[id].exists = 1;
			}
			// store foliage/grass colour values
			else if (i >= FOLIAGE_R && i <= FOLIAGE_B)
				(*biomes)[id].foliage[i - FOLIAGE_R] = (uint8_t)strtol(value, NULL, 0);
			else if (i >= GRASS_R && i <= GRASS_B)
				(*biomes)[id].grass[i - GRASS_R] = (uint8_t)strtol(value, NULL, 0);
		}
	}

	return max_biomeid + 1;
}


// convert a 24-bit RGB colour to float-based HSV
static void rgb2hsv(const uint8_t *rgbint, double *hsv)
{
	double r = rgbint[0] / 255.0;
	double g = rgbint[1] / 255.0;
	double b = rgbint[2] / 255.0;
	double rgb_min = g < b ? (r < g ? r : g) : (r < b ? r : b);
	double rgb_max = g > b ? (r > g ? r : g) : (r > b ? r : b);
	double range = rgb_max - rgb_min;

	// find value
	hsv[2] = rgb_max;
	if (hsv[2] == 0) // black
	{
		hsv[0] = hsv[1] = 0;
		return;
	}

	// find saturation
	hsv[1] = range / rgb_max;
	if (hsv[1] == 0) // grey
	{
		hsv[0] = 0;
		return;
	}

	// find hue
	if (r == rgb_max)
		hsv[0] = (g - b) / range;
	else if (g == rgb_max)
		hsv[0] = (b - r) / range + 2.0;
	else
		hsv[0] = (r - g) / range + 4.0;
	hsv[0] *= 60.0;
	if (hsv[0] < 0.0) hsv[0] += 360.0;
}


// convert a float-based HSV colour to 24-bit RGB
static void hsv2rgb(const double *hsv, uint8_t *rgbint)
{
	double r, g, b;

	if (hsv[1] == 0) // grey
	{
		r = g = b = hsv[2];
		return;
	}

	double h = hsv[0] / 60; // hue, 0 to 6
	uint8_t i = (uint8_t)h;
	double f = h - i; // fractional hue
	double p = hsv[2] * (1 - hsv[1]);
	double q = hsv[2] * (1 - hsv[1] * f);
	double t = hsv[2] * (1 - hsv[1] * (1 - f));

	switch(i) {
	case 0:
		r = hsv[2];
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = hsv[2];
		b = p;
		break;
	case 2:
		r = p;
		g = hsv[2];
		b = t;
		break;
	case 3:
		r = p;
		g = q;
		b = hsv[2];
		break;
	case 4:
		r = t;
		g = p;
		b = hsv[2];
		break;
	default: // 5
		r = hsv[2];
		g = p;
		b = q;
		break;
	}

	rgbint[0] = (uint8_t)(r * 255);
	rgbint[1] = (uint8_t)(g * 255);
	rgbint[2] = (uint8_t)(b * 255);
}


// lighten and darken an RGB colour and store the results in memory immediately afterward
static void add_hilight_and_shadow(uint8_t *colour)
{
	// copy the main colour to the hilight and shadow colours
	memcpy(colour + CHANNELS, colour, CHANNELS);
	memcpy(colour + CHANNELS * 2, colour, CHANNELS);
	// adjust the hilight and shadow colours
	adjust_colour_brightness(colour + CHANNELS, HILIGHT_AMOUNT);
	adjust_colour_brightness(colour + CHANNELS * 2, SHADOW_AMOUNT);
}


// adjust a block colour's hue toward that of a biome colour
static void mix_biome_colour(uint8_t *biome_block_colour, const uint8_t *block_colour,
		biome *biome, const uint8_t biome_colourtype)
{
	if (biome_colourtype > 0)
	{
		// generate biome colour
		uint8_t *biome_colour = biome_colourtype == 1 ? biome->foliage : biome->grass;

		double block_hsv[3], biome_hsv[3];
		rgb2hsv(block_colour, block_hsv);
		rgb2hsv(biome_colour, biome_hsv);

		// use hue/sat from biome colour, and val from block colour
		double biome_block_hsv[3] = {
			biome_hsv[0],
			biome_hsv[1],
			block_hsv[2]
		};
		hsv2rgb(biome_block_hsv, biome_block_colour);
		biome_block_colour[ALPHA] = block_colour[ALPHA];

		// create highlight and shadow colours
		add_hilight_and_shadow(biome_block_colour);
	}
	// or just copy all the colours from the regular block palette
	else memcpy(biome_block_colour, block_colour, CHANNELS * 3);
}


textures *read_textures(const char *texpath, const char *shapepath, const char *biomepath)
{
	textures *tex = (textures*)calloc(1, sizeof(textures));

	shape *shapes = NULL;
	if (shapepath != NULL) read_shapes(&shapes, shapepath);

	biome *biomes = NULL;
	uint8_t biomecount;
	if (biomepath != NULL) biomecount = read_biomes(&biomes, biomepath);

	// colour/texture file
	FILE *tcsv = fopen(texpath, "r");
	if (tcsv == NULL) printf("Error %d reading texture file: %s\n", errno, texpath);
	char line[LINE_BUFFER];

	// find highest block id and allocate blocktypes array
	tex->max_blockid = -1;
	while (fgets(line, LINE_BUFFER, tcsv))
		tex->max_blockid = (int)strtol(strtok(line, ","), NULL, 0);
	tex->blockids = (blockID*)calloc(tex->max_blockid + 1, sizeof(blockID));

	// read block textures
	fseek(tcsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, tcsv))
	{
		uint8_t row[TEXCOL_COUNT];
		char *pos = line;
		for (uint8_t i = 0; i < TEXCOL_COUNT; i++)
		{
			// read the length of the next value
			uint8_t len = (pos < line + LINE_BUFFER) ? strcspn(pos, ",") : 0;
			// store the value in the buffer
			char value[LINE_BUFFER];
			strncpy(value, pos, len);
			*(value + len) = '\0';
			// advance the pointer, unless we are at the end of the buffer
			if (pos < line + LINE_BUFFER) pos += len + 1;

			// parse the value as an integer
			row[i] = strcmp(value, "") ? (uint8_t)strtol(value, NULL, 0) : 0;
		}

		// subtype mask is specified on one (generally the first) subtype of each block type
		if (row[SUBTYPE_MASK]) tex->blockids[row[BLOCKID]].subtype_mask = row[SUBTYPE_MASK];

		// copy values for this block type
		blocktype *btype = &tex->blockids[row[BLOCKID]].subtypes[row[SUBTYPE]];
		btype->id = row[BLOCKID];
		btype->subtype = row[SUBTYPE];

		// copy colours and adjust
		memcpy(&btype->palette[COLOUR1], &row[RED1], CHANNELS);
		add_hilight_and_shadow(btype->palette[COLOUR1]);
		memcpy(&btype->palette[COLOUR2], &row[RED2], CHANNELS);
		add_hilight_and_shadow(btype->palette[COLOUR2]);

		// check if this block type uses biome colours
		if (biomepath != NULL && (row[BIOME_COLOUR1] || row[BIOME_COLOUR2]))
		{
			// calculate colours for this block type in every biome
			btype->biome_palettes = (palette*)calloc(biomecount, sizeof(palette));

			for (uint8_t b = 0; b < biomecount; b++)
				if (biomes[b].exists)
				{
					mix_biome_colour(btype->biome_palettes[b][COLOUR1], btype->palette[COLOUR1],
							&biomes[b], row[BIOME_COLOUR1]);
					mix_biome_colour(btype->biome_palettes[b][COLOUR2], btype->palette[COLOUR2],
							&biomes[b], row[BIOME_COLOUR2]);
				}
		}
		else btype->biome_palettes = NULL;

		if (shapepath != NULL)
		{
			// copy shapes for each rotation; if any of the last 3 are blank or zero, use the first
			btype->shapes[0] = shapes[row[SHAPE_N]];
			btype->shapes[1] = shapes[row[SHAPE_E] || row[SHAPE_N]];
			btype->shapes[2] = shapes[row[SHAPE_S] || row[SHAPE_N]];
			btype->shapes[3] = shapes[row[SHAPE_W] || row[SHAPE_N]];
		}
	}
	fclose(tcsv);

	free(shapes);
	free(biomes);

	return tex;
}


void free_textures(textures *tex)
{
	for (int b = 0; b <= tex->max_blockid; b++)
		for (int s = 0; s < BLOCK_SUBTYPES; s++)
			free(tex->blockids[b].subtypes[s].biome_palettes);
	free(tex->blockids);
	free(tex);
}


const blocktype *get_block_type(const textures *tex, const uint8_t blockid, const uint8_t dataval)
{
	return &tex->blockids[blockid].subtypes[dataval % tex->blockids[blockid].subtype_mask];
}


void adjust_colour_brightness(uint8_t *pixel, float mod)
{
	if (pixel[ALPHA] == 0) return;

	for (uint8_t c = 0; c < ALPHA; c++)
		// room to adjust, multiplied by modifier %, gives us the amount to adjust
		pixel[c] += (mod < 0 ? pixel[c] : 255 - pixel[c]) * mod;
}


void combine_alpha(uint8_t *top, uint8_t *bottom, bool down)
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
	uint8_t alpha = top[ALPHA] + bottom[ALPHA] * bmod;
	uint8_t *target = down ? bottom : top;
	for (uint8_t ch = 0; ch < ALPHA; ch++)
		target[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
	target[ALPHA] = alpha;
}
