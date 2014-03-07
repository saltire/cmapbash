#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colours.h"


unsigned char* read_colours(const char* colourfile)
{
	FILE* csv = fopen(colourfile, "r");
	if (csv == NULL)
	{
		printf("Error %d reading colour file: %s\n", errno, colourfile);
	}

	unsigned char* colours = (unsigned char*)calloc(BLOCK_TYPES * CHANNELS, sizeof(char));

	char* line = (char*)malloc(60);
	while (fgets(line, 60, csv))
	{
		unsigned char id;
		for (int i = 0; i < 2 + CHANNELS; i++)
		{
			char* value = strtok(i == 0 ? line : NULL, ",");
			unsigned char num = (value == NULL ? 0 : (char)strtol(value, NULL, 0));

			if (i == 0)
			{
				id = num;
				//printf("id = %d\n", num);
			}
			else if (i == 1 && num != 0)
			{
				//printf("Data type = %d; breaking\n", (int)num);
				break;
			}
			else if (i > 1)
			{
				//printf("setting %d (%d) to %d\n", id * 4 + i - 2, (int)&(colours[id * 4 + i - 2]), num);
				colours[id * CHANNELS + i - 2] = num;
				//puts("done");
			}
		}
	}
	free(line);

	fclose(csv);
	return colours;
}



