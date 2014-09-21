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


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "world.h"


int main(int argc, char **argv)
{
	char *inpath = NULL;
	char *outpath = "map.png";
	static options opts =
	{
		.isometric = 0,
		.night     = 0,
		.shadows   = 0,
		.tiny      = 0,
		.rotate    = 0,
		.texpath   = "textures.csv",
		.shapepath = "shapes.csv"
	};
	unsigned int rotateint;

	// flush output on newlines
	setvbuf(stdout, NULL, _IOLBF, 0);

	static struct option long_options[] =
	{
		{"isometric", no_argument, &opts.isometric, 1},
		{"night",     no_argument, &opts.night,     1},
		{"shadows",   no_argument, &opts.shadows,   1},
		{"tiny",      no_argument, &opts.tiny,      1},
		{"rotate",    required_argument, 0, 'r'},
		{"world",     required_argument, 0, 'w'},
		{"output",    required_argument, 0, 'o'},
		{0, 0, 0, 0}
	};

	int c;
	while (1)
	{
		int option_index = 0;
		c = getopt_long(argc, argv, "instr:w:o:", long_options, &option_index);
		if (c == -1) break;

		switch (c)
		{
		case 0: // flag set with long option
			break;
		case 'i':
			opts.isometric = 1;
			break;
		case 'n':
			opts.night = 1;
			break;
		case 's':
			opts.shadows = 1;
			break;
		case 't':
			opts.tiny = 1;
			break;
		case 'r':
			if (sscanf(optarg, "%d", &rotateint))
				opts.rotate = (unsigned char)rotateint % 4;
			else
				fprintf(stderr, "Invalid rotate argument: %s\n", optarg);
			break;
		case 'w':
			inpath = optarg;
			break;
		case 'o':
			outpath = optarg;
			break;
		case '?': // unknown option
			break;
		default:
			abort();
		}
	}

	if (inpath == NULL)
	{
		printf("Please specify a region directory with -w.\n");
		return 1;
	}

	if (opts.rotate)
		printf("Rotating %d degrees clockwise\n", opts.rotate * 90);

	if (opts.tiny)
	{
		printf("Rendering in tiny mode\n");
		save_tiny_world_map(inpath, outpath, opts);
	}
	else
	{
		printf("Rendering in %s mode\n", opts.isometric ? "isometric" : "orthographic");
		if (opts.night)
			printf("Night mode is on\n");
		else if (opts.isometric && opts.shadows)
			printf("Daytime shadows are on\n");
		save_world_map(inpath, outpath, opts);
	}

	return 0;
}
