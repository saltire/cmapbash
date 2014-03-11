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
