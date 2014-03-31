#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shapes.h"
#include "textures.h"


texture* read_textures(const char* texturefile)
{
	FILE* csv = fopen(texturefile, "r");
	if (csv == NULL)
	{
		printf("Error %d reading colour file: %s\n", errno, texturefile);
	}

	texture* textures = (texture*)calloc(BLOCK_TYPES, sizeof(texture));

	char* line = (char*)malloc(60);
	while (fgets(line, 60, csv))
	{
		unsigned char id, mask, type;
		for (int i = 0; i < 9; i++)
		{
			char* value = strtok(i == 0 ? line : NULL, ",");
			unsigned char num = (value == NULL ? 0 : (char)strtol(value, NULL, 0));

			if (i == 0)
			{
				id = num;
			}
			else if (i == 1)
			{
				mask = num;
			}
			else if (i == 2)
			{
				type = num;
				if (type == 0)
				{
					textures[id].mask = mask;
				}
			}
			else if (i >= 3 && i <= 6)
			{
				textures[id].types[type].colour[i - 3] = num;
			}
			else if (i == 8)
			{
				memcpy(&textures[id].types[type].shape, &shapes[num],
						ISO_BLOCK_WIDTH * ISO_BLOCK_HEIGHT);
			}
		}
	}
	free(line);

	fclose(csv);
	return textures;
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

	if (down)
	{
		// store the blended colour in the bottom colour's buffer
		for (int ch = 0; ch < ALPHA; ch++)
		{
			bottom[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
		}
		bottom[ALPHA] = alpha;
	}
	else
	{
		// store the blended colour in the top colour's buffer
		for (int ch = 0; ch < ALPHA; ch++)
		{
			top[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
		}
		top[ALPHA] = alpha;
	}
}
