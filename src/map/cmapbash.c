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

#include "data.h"
#include "map.h"


int main(int argc, char **argv)
{
	char *inpath = NULL;
	char *outpath = "map.png";
	char *action = "generate";
	static options opts =
	{
		.limits    = NULL,
		.ylimits   = NULL,
		.texpath   = "resources/textures.csv",
		.shapepath = "resources/shapes.csv",
		.biomepath = "resources/biomes.csv",
	};
	unsigned int rotateint;
	int fc = 0;
	int f1, f2, f3, fx, fy, fz;
	int tc = 0;
	int t1, t2, t3, tx, ty, tz;

	// flush output on newlines
	setvbuf(stdout, NULL, _IOLBF, 0);

	static struct option long_options[] =
	{
		{"isometric", no_argument, &opts.isometric, 1},
		{"night",     no_argument, &opts.night,     1},
		{"shadows",   no_argument, &opts.shadows,   1},
		{"biomes",    no_argument, &opts.biomes,    1},
		{"tiny",      no_argument, &opts.tiny,      1},
		{"rotate",    required_argument, 0, 'r'},
		{"world",     required_argument, 0, 'w'},
		{"output",    required_argument, 0, 'o'},
		{"from",      required_argument, 0, 'F'},
		{"to",        required_argument, 0, 'T'},
		{0, 0, 0, 0}
	};

	int c;
	int first = 1;
	while (1)
	{
		int option_index = 2;
		c = getopt_long(argc, argv, "-insbtr:w:o:F:T:", long_options, &option_index);
		if (c == -1) break;

		switch (c)
		{
		case 0: // flag set with long option
		case '?': // unknown option
			break;

		case '\1': // non-option
			if (first) action = optarg;
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

		case 'b':
			opts.biomes = 1;
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

		case 'F':
			fc = sscanf(optarg, "%d,%d,%d", &f1, &f2, &f3);
			if (fc < 2)
				fprintf(stderr, "Invalid 'from' coordinates: %s\n", optarg);
			break;

		case 'T':
			tc = sscanf(optarg, "%d,%d,%d", &t1, &t2, &t3);
			if (tc < 2)
				fprintf(stderr, "Invalid 'to' coordinates: %s\n", optarg);
			break;

		default:
			abort();
		}

		first = 0;
	}

	if (action == "generate")
		printf("Generating map.\n");
	else
	{
		printf("Unknown action.\n");
		return 1;
	}

	if (inpath == NULL)
	{
		printf("Please specify a region directory with -w.\n");
		return 1;
	}

	if (fc != tc)
		fprintf(stderr, "'From' and 'to' coordinates must be in the same format (X,Z or X,Y,Z).\n");
	else if (fc >= 2)
	{
		fx = f1;
		tx = t1;
		if (fc == 3)
		{
			fz = f3;
			tz = t3;

			int ylimits[2] =
			{
				MAX(0, MIN(f2,t2)),
				MIN(MAX_HEIGHT, MAX(f2,t2)),
			};
			opts.ylimits = ylimits;
		}
		else
		{
			fz = f2;
			tz = t2;
		}

		int limits[4] =
		{
			MIN(fz,tz),
			MAX(fx,tx),
			MAX(fz,tz),
			MIN(fx,tx),
		};
		opts.limits = limits;

		if (opts.ylimits == NULL)
			printf("Drawing from (X:%d, Z:%d) to (X:%d, Z:%d)\n",
					fx, fz, tx, tz);
		else
			printf("Drawing from (X:%d, Y:%d, Z:%d) to (X:%d, Y:%d, Z:%d)\n",
					fx, opts.ylimits[0], fz, tx, opts.ylimits[1], tz);
	}

	if (opts.rotate) printf("Rotating %d degrees clockwise\n", opts.rotate * 90);

	if (opts.tiny)
	{
		printf("Rendering in tiny mode.\n");
		opts.isometric = 0;
	}
	else
	{
		printf("Rendering in %s mode\n", opts.isometric ? "isometric" : "orthographic");

		if (opts.night) printf("Night mode is on\n");
		else if (opts.isometric && opts.shadows) printf("Daytime shadows are on\n");

		if (opts.biomes) printf("Biomes are on\n");
	}

	save_world_map(inpath, outpath, &opts);

	return 0;
}

int slice_test(int argc, char **argv)
{
	image *img = load_image("map-ortho-day.png");

	slice_image(img, 512, "tiles");

	free_image(img);

	return 0;
}
