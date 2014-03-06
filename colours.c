#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BLOCK_TYPES 176


unsigned char* read_colours(const char* filename)
{
	FILE* csv = fopen(filename, "r");
	if (csv == NULL)
	{
		printf("Error reading file!\n");
	}

	unsigned char* colours = (unsigned char*)malloc(BLOCK_TYPES * 4);

	char* line = (char*)malloc(60);
	while (fgets(line, 60, csv))
	{
		unsigned char id;
		for (int i = 0; i < 6; i++)
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
				colours[id * 4 + i - 2] = num;
				//puts("done");
			}
		}
	}
	//free(line);
	return colours;
}



