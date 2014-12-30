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

#include "image.h"
#include "map.h"
#include "textures.h"


#define LINE_BUFFER 100


typedef enum
{
	BLOCKID,
	MASK,
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
	FOLIAGE_A,
	GRASS_R,
	GRASS_G,
	GRASS_B,
	GRASS_A,
	BIOMECOL_COUNT
}
biomecols;


static void read_shapes(shape **shapes, const char *shapepath)
{
	// shape file
	FILE *scsv = fopen(shapepath, "r");
	if (scsv == NULL) printf("Error %d reading shape file: %s\n", errno, shapepath);
	char line[LINE_BUFFER];

	// count shapes and allocate array
	int c, scount = 0;
	while ((c = getc(scsv)) != EOF) if (c == '\n') scount++;
	*shapes = (shape*)calloc(scount, sizeof(shape));

	// read shape pixel maps
	int s = 0;
	fseek(scsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, scsv))
	{
		for (int p = 0; p < ISO_BLOCK_AREA; p++)
		{
			// convert ascii value to numeric value
			unsigned char pcolour = line[p] - '0';
			(*shapes)[s].pixels[p] = pcolour;
			(*shapes)[s].has[pcolour] = 1;
		}
		(*shapes)[s].is_solid = !((*shapes)[s].has[BLANK] == 1);
		s++;
	}
	fclose(scsv);
}


static int read_biomes(biome **biomes, const char *biomepath)
{
	// biome file
	FILE *bcsv = fopen(biomepath, "r");
	if (bcsv == NULL) printf("Error %d reading biome file: %s\n", errno, biomepath);
	char line[LINE_BUFFER];

	// find highest biome id and allocate biomes array
	int max_biomeid = -1;
	while (fgets(line, LINE_BUFFER, bcsv))
		max_biomeid = (int)strtol(strtok(line, ","), NULL, 0);
	*biomes = (biome*)calloc(max_biomeid + 1, sizeof(biome));

	// read biome colours
	fseek(bcsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, bcsv))
	{
		unsigned char id;
		char *pos = line;
		for (int i = 0; i < BIOMECOL_COUNT; i++)
		{
			// read the length of the next value
			size_t len = (pos < line + LINE_BUFFER) ? strcspn(pos, ",") : 0;

			// read the value string
			char value[LINE_BUFFER];
			strncpy(value, pos, len);
			*(value + len) = '\0';

			// remember id
			if (i == BIOMEID)
			{
				id = (unsigned char)strtol(value, NULL, 0);
				(*biomes)[id].exists = 1;
			}

			// store foliage/grass colour values
			else if (i >= FOLIAGE_R && i <= FOLIAGE_A)
				(*biomes)[id].foliage[i - FOLIAGE_R] = (unsigned char)strtol(value, NULL, 0);
			else if (i >= GRASS_R && i <= GRASS_A)
				(*biomes)[id].grass[i - GRASS_R] = (unsigned char)strtol(value, NULL, 0);

			// advance the pointer, unless we are at the end of the buffer
			if (pos < line + LINE_BUFFER) pos += len + 1;
		}
	}

	return max_biomeid + 1;
}


textures *read_textures(const char *texpath, const char *shapepath, const char *biomepath)
{
	textures *tex = (textures*)calloc(1, sizeof(textures));

	shape *shapes;
	if (shapepath != NULL) read_shapes(&shapes, shapepath);

	int biomecount = -1;
	if (biomepath != NULL) biomecount = read_biomes(&tex->biomes, biomepath);

	// colour file
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
		unsigned char row[TEXCOL_COUNT];
		char *pos = line;
		for (int i = 0; i < TEXCOL_COUNT; i++)
		{
			// read the length of the next value
			size_t len = (pos < line + LINE_BUFFER) ? strcspn(pos, ",") : 0;

			// read the value string and parse it as an integer
			char value[LINE_BUFFER];
			strncpy(value, pos, len);
			*(value + len) = '\0';
			row[i] = strcmp(value, "") ? (unsigned char)strtol(value, NULL, 0) : 0;

			// advance the pointer, unless we are at the end of the buffer
			if (pos < line + LINE_BUFFER) pos += len + 1;
		}

		// subtype mask is specified on subtype 0 of each block type
		if (row[SUBTYPE] == 0) tex->blockids[row[BLOCKID]].mask = row[MASK];

		// copy values for this block type
		blocktype *btype = &tex->blockids[row[BLOCKID]].subtypes[row[SUBTYPE]];
		btype->id = row[BLOCKID];
		btype->subtype = row[SUBTYPE];
		btype->is_opaque = (row[ALPHA1] == 255 && (row[ALPHA2] == 255 || row[ALPHA2] == 0));
		//btype->biome_colour = row[BIOME_COLOUR];

		if (shapepath != NULL)
		{
			// copy shapes for each rotation; if any of the last 3 are blank or zero, use the first
			btype->shapes[0] = shapes[row[SHAPE_N]];
			btype->shapes[1] = shapes[row[SHAPE_E] || row[SHAPE_N]];
			btype->shapes[2] = shapes[row[SHAPE_S] || row[SHAPE_N]];
			btype->shapes[3] = shapes[row[SHAPE_W] || row[SHAPE_N]];

			for (int i = 0; i < COLOUR_COUNT; i++)
			{
				btype->has[i] = (btype->shapes[0]->has[i] & btype->shapes[1]->has[i] &
						btype->shapes[2]->has[i] & btype->shapes[3]->has[i]);
			}
		}

		// copy colours and adjust
		memcpy(&btype->colours[COLOUR1], &row[RED1], CHANNELS);
		memcpy(&btype->colours[COLOUR2], &row[RED2], CHANNELS);
		add_hilight_and_shadow(&btype->colours[COLOUR1]);
		add_hilight_and_shadow(&btype->colours[COLOUR2]);

		if (biomepath != NULL)
		{
			// allocate biome colours array for this block type
			btype->biome_colours = (unsigned char**)calloc(biomecount, sizeof(char**));

			for (int b = 0; b < biomecount; b++)
				if (tex->biomes[b].exists)
				{
					btype->biome_colours[b] = (unsigned char*)calloc(
							COLOUR_COUNT * CHANNELS * sizeof(char));
					unsigned char *colour1 = &btype->biome_colours[b][COLOUR1 * CHANNELS];
					unsigned char *colour2 = &btype->biome_colours[b][COLOUR2 * CHANNELS];

					if (row[BIOME_COLOUR1] > 0)
					{
						memcpy(colour1, &row[RED1], CHANNELS);
						combine_alpha(row[BIOME_COLOUR1] == 1 ?
								tex->biomes[b].foliage : tex->biomes[b].grass, colour1, 1);
						add_colour_and_shadow(colour2);
					}
					if (row[BIOME_COLOUR2] > 0)
					{
						memcpy(colour2, &row[RED2], CHANNELS);
						combine_alpha(row[BIOME_COLOUR2] == 1 ?
								tex->biomes[b].foliage : tex->biomes[b].grass, colour2, 1);
						add_colour_and_shadow(colour2);
					}
				}
		}
	}
	fclose(tcsv);

	free(shapes);

	return tex;
}


void add_hilight_and_shadow(unsigned char *colour)
{
	memcpy(colour + CHANNELS, colour, CHANNELS);
	memcpy(colour + (CHANNELS * 2), colour, CHANNELS);
	adjust_colour_brightness(colour + CHANNELS, HILIGHT_AMOUNT);
	adjust_colour_brightness(colour + (CHANNELS * 2), SHADOW_AMOUNT);
}


void free_textures(textures *tex)
{
	free(tex->blockids);
	free(tex->biomes);
	free(tex);
}


const blocktype *get_block_type(const textures *tex,
		const unsigned char blockid, const unsigned char dataval)
{
	return &tex->blockids[blockid].subtypes[dataval % tex->blockids[blockid].mask];
}


void set_colour_brightness(unsigned char *pixel, float brightness, float ambience)
{
	if (pixel[ALPHA] == 0) return;

	for (int c = 0; c < ALPHA; c++)
		// darken pixel to ambient light only, then add brightness
		pixel[c] *= (ambience + (1 - ambience) * brightness);
}


void adjust_colour_brightness(unsigned char *pixel, float mod)
{
	if (pixel[ALPHA] == 0) return;

	for (int c = 0; c < ALPHA; c++)
		// room to adjust, multiplied by modifier %, gives us the amount to adjust
		pixel[c] += (mod < 0 ? pixel[c] : 255 - pixel[c]) * mod;
}


void combine_alpha(unsigned char *top, unsigned char *bottom, int down)
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
	unsigned char *target = down ? bottom : top;
	for (int ch = 0; ch < ALPHA; ch++)
		target[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
	target[ALPHA] = alpha;
}


void rgb2hsv(unsigned char *rgbint, double *hsv)
{
	double rgb[3] = {
		(double)rgbint[0] / 255.0,
		(double)rgbint[1] / 255.0,
		(double)rgbint[2] / 255.0
	};
	double rgb_min, rgb_max;

	// find value
	hsv[2] = MAX3(rgb[0], rgb[1], rgb[2]);
	if (hsv[2] == 0)
	{
		hsv[0] = hsv[1] = 0;
		return;
	}

	// normalize to value 1
	rgb[0] /= hsv[2];
	rgb[1] /= hsv[2];
	rgb[2] /= hsv[2];
	rgb_min = MIN3(rgb[0], rgb[1], rgb[2]);
	rgb_max = MAX3(rgb[0], rgb[1], rgb[2]);

	// find saturation
	hsv[1] = rgb_max - rgb_min;
	if (hsv[1] == 0)
	{
		hsv[0] = 0;
		return;
	}

	// normalize to saturation 1
	double range = rgb_max - rgb_min;
	rgb[0] = (rgb[0] - rgb_min) / range;
	rgb[1] = (rgb[1] - rgb_min) / range;
	rgb[2] = (rgb[2] - rgb_min) / range;
	rgb_min = MIN3(rgb[0], rgb[1], rgb[2]);
	rgb_max = MAX3(rgb[0], rgb[1], rgb[2]);

	if (rgb[0] == rgb_max)
		hsv[0] = (360.0 + 60.0 * (rgb[1] - rgb[2])) % 360.0;
	else if (rgb[1] == rgb_max)
		hsv[0] = 120.0 + 60.0 * (rgb[2] - rgb[0]);
	else
		hsv[0] = 240.0 + 60.0 * (rgb[0] - rgb[1]);
}
