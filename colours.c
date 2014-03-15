#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colours.h"


colour* read_colours(const char* colourfile)
{
	FILE* csv = fopen(colourfile, "r");
	if (csv == NULL)
	{
		printf("Error %d reading colour file: %s\n", errno, colourfile);
	}

	colour* colours = (colour*)calloc(BLOCK_TYPES, sizeof(colour));

	char* line = (char*)malloc(60);
	while (fgets(line, 60, csv))
	{
		unsigned char id, mask, type;
		for (int i = 0; i < 3 + CHANNELS; i++)
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
					colours[id].mask = mask;
				}
			}
			else if (i > 2)
			{
				colours[id].types[type * CHANNELS + i - 3] = num;
			}
		}
	}
	free(line);

	fclose(csv);
	return colours;
}


void adjust_colour_by_lum(unsigned char* pixel, unsigned char light)
{
	for (int c = 0; c < ALPHA; c++)
	{
		float darkness = 0.2;
		float lightness = (float)light / 15;
		//printf("Rendering pixel with lightness %d (%f): %d -> ", light, lightness, pixel[c]);
		pixel[c] = pixel[c] * (darkness + (1 - darkness) * lightness);
		//printf("%d\n", pixel[c]);
	}
}


void adjust_colour_by_height(unsigned char* pixel, int y)
{
	for (int c = 0; c < ALPHA; c++)
	{
		// linear height shader
		//float contrast = 0.8;
		//pixel[c] = pixel[c] * contrast + y * (1 - contrast);

		// curved, higher contrast height shader
		float contrast = 0.7; // amount of contrast
		float center = 0.3; // gamma correction

		int alt = y - (center * 255); // +/- distance from center
		unsigned char limit = alt < 0 ? pixel[c] : 255 - pixel[c]; // room to adjust
		int mod = limit * (float)alt /
				(alt < 0 ? center * 255 : (1 - center) * 255); // amount to adjust
		pixel[c] = pixel[c] + mod * contrast;
	}
}


void combine_alpha(unsigned char* top, unsigned char* bottom, int down)
{
	if (top[ALPHA] == 255 || bottom[ALPHA] == 0)
	{
		if (down && top[ALPHA] > 0)
		{
			memcpy(bottom, top, CHANNELS);
		}
		return;
	}
	if (top[ALPHA] == 0)
	{
		if (!down && bottom[ALPHA] > 0)
		{
			memcpy(top, bottom, CHANNELS);
		}
		return;
	}

	float bmod = (float)(255 - top[ALPHA]) / 255;
	unsigned char alpha = top[ALPHA] + bottom[ALPHA] * bmod;

	for (int ch = 0; ch < ALPHA; ch++)
	{
		// if down is true, store in the bottom colour's buffer, otherwise the top
		if (down)
		{
			bottom[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
		}
		else
		{
			top[ch] = (top[ch] * top[ALPHA] + bottom[ch] * bottom[ALPHA] * bmod) / alpha;
		}
	}
	down ? (bottom[ALPHA] = alpha) : (top[ALPHA] = alpha);
}
