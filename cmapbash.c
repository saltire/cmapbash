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


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "textures.h"
#include "world.h"


int main(int argc, char **argv)
{
	char *inpath = NULL;
	char *outpath = "map.png";
	options opts = {
			.isometric = 0,
			.night = 0,
			.quick = 0,
			.rotate = 0,
			.texpath = "textures.csv",
			.shapepath = "shapes.csv"
	};

	// flush output on newlines
	setvbuf(stdout, NULL, _IOLBF, 0);

	// parse options
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i"))
			opts.isometric = 1;

		else if (!strcmp(argv[i], "-n"))
			opts.night = 1;

		else if (!strcmp(argv[i], "-q"))
			opts.quick = 1;

		else if (!strcmp(argv[i], "-r") && i + 1 < argc)
		{
			unsigned int rotateint;
			if (sscanf(argv[i + 1], "%d", &rotateint))
				opts.rotate = (unsigned char)rotateint % 4;
			else
				printf("Invalid rotate value: %s\n", argv[i + 1]);
			i++;
		}

		else if (!strcmp(argv[i], "-o") && i + 1 < argc)
		{
			outpath = argv[i + 1];
			i++;
		}

		else if (inpath == NULL)
			inpath = argv[i];
	}

	if (inpath == NULL)
	{
		printf("Please specify a region directory.\n");
		return 0;
	}

	if (opts.rotate)
		printf("Rotating %d degrees clockwise\n", opts.rotate * 90);

	if (opts.quick)
	{
		opts.isometric = 0;
		printf("Rendering in quick mode\n");
	}
	else
	{
		printf("Rendering in %s mode\n", opts.isometric ? "isometric" : "orthographic");
		if (opts.night)
			printf("Night mode is on\n");
	}

	save_world_map(inpath, outpath, opts);

	return 0;
}

