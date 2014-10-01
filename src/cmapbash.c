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
#include "dims.h"
#include "world.h"


int main(int argc, char **argv)
{
	char *inpath = NULL;
	char *outpath = "map.png";
	static options opts =
	{
		.texpath   = "resources/textures.csv",
		.shapepath = "resources/shapes.csv",
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
		{"tiny",      no_argument, &opts.tiny,      1},
		{"rotate",    required_argument, 0, 'r'},
		{"world",     required_argument, 0, 'w'},
		{"output",    required_argument, 0, 'o'},
		{"from",      required_argument, 0, 'F'},
		{"to",        required_argument, 0, 'T'},
		{0, 0, 0, 0}
	};

	int c;
	while (1)
	{
		int option_index = 0;
		c = getopt_long(argc, argv, "instr:w:o:F:T:", long_options, &option_index);
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

	if (fc != tc)
		fprintf(stderr, "'From' and 'to' coordinates must be in the same format (X,Z or X,Y,Z).\n");
	else if (fc >= 2)
	{
		fx = f1;
		tx = t1;
		if (fc == 3)
		{
			fy = f2;
			ty = t2;
			fz = f3;
			tz = t3;
		}
		else
		{
			fy = 0;
			ty = MAX_HEIGHT;
			fz = f2;
			tz = t2;
		}

		opts.use_limits = 1;
		opts.limits[0] = MIN(fz,tz);
		opts.limits[1] = MAX(fx,tx);
		opts.limits[2] = MAX(fz,tz);
		opts.limits[3] = MIN(fx,tx);
		opts.ymin = MAX(0, MIN(fy,ty));
		opts.ymax = MIN(MAX_HEIGHT, MAX(fy,ty));

		printf("Drawing from (X:%d, Y:%d, Z:%d) to (X:%d, Y:%d, Z:%d)\n",
				fx, fy, fz, tx, ty, tz);
	}

	if (opts.rotate)
		printf("Rotating %d degrees clockwise\n", opts.rotate * 90);

	if (opts.tiny)
	{
		printf("Rendering in tiny mode.\n");
		save_tiny_world_map(inpath, outpath, &opts);
	}
	else
	{
		printf("Rendering in %s mode\n", opts.isometric ? "isometric" : "orthographic");
		if (opts.night)
			printf("Night mode is on\n");
		else if (opts.isometric && opts.shadows)
			printf("Daytime shadows are on\n");
		save_world_map(inpath, outpath, &opts);
	}

	return 0;
}
