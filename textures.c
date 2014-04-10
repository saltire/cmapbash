#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "textures.h"


#define COLUMNS 9
#define LINE_BUFFER 60


textures* read_textures(const char* texturefile, const char* shapefile)
{
	textures* tex = (textures*)calloc(1, sizeof(textures));

	char line[LINE_BUFFER];

	// colour file
	FILE* tcsv = fopen(texturefile, "r");
	if (tcsv == NULL) printf("Error %d reading texture file: %s\n", errno, texturefile);

	// find highest block id and allocate blocktypes array
	unsigned char blockid = -1;
	while (fgets(line, LINE_BUFFER, tcsv)) blockid = (char)strtol(strtok(line, ","), NULL, 0);
	tex->blockcount = blockid + 1;
	tex->blocktypes = (blocktype*)calloc(tex->blockcount, sizeof(blocktype));

	// read block textures
	fseek(tcsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, tcsv))
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
	fclose(tcsv);

	// shape file
	FILE* scsv = fopen(shapefile, "r");
	if (scsv == NULL) printf("Error %d reading shape file: %s\n", errno, shapefile);

	// count shapes and allocate array
	int c, s = 0;
	while ((c = getc(scsv)) != EOF) if (c == '\n') s++;
	tex->shapes = (unsigned char*)calloc(s * ISO_BLOCK_AREA, sizeof(char));

	// read shape maps
	s = 0;
	fseek(scsv, 0, SEEK_SET);
	while (fgets(line, LINE_BUFFER, scsv))
	{
		// convert ascii value to numeric value
		for (int i = 0; i < ISO_BLOCK_AREA; i++)
			tex->shapes[s * ISO_BLOCK_AREA + i] = line[i] - '0';
		s++;
	}
	fclose(scsv);

	return tex;
}


void free_textures(textures* tex)
{
	free(tex->blocktypes);
	free(tex->shapes);
	free(tex);
}


const unsigned char* get_block_colour(const textures* tex,
		const unsigned char blockid, const unsigned char dataval)
{
	unsigned char type = dataval % tex->blocktypes[blockid].mask;
	return tex->blocktypes[blockid].subtypes[type].colour;
}


const unsigned char get_block_shapeid(const textures* tex,
		const unsigned char blockid, const unsigned char dataval)
{
	unsigned char type = dataval % tex->blocktypes[blockid].mask;
	return tex->blocktypes[blockid].subtypes[type].shapeid;
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
