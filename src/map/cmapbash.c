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


#define _XOPEN_SOURCE 500 // for FTW

#include <ftw.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data.h"
#include "map.h"
#include "image.h"


#define TILESIZE 1024


// save the map to a PNG file
static void save_world_map_image(image *img, const char *imgpath)
{
	printf("Saving image to %s ...\n", imgpath);
	clock_t start = clock();
	save_image(img, imgpath);
	printf("Total save time: %f seconds\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}


// nftw function to remove a file
static int rm_file(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
	remove(fpath);
	return 0;
}


// slice the map into a set of tiles for use with google maps
static void save_world_map_slices(image *img, char *slicepath)
{
	// strip trailing slash
	size_t dirlen = strlen(slicepath);
	if (slicepath[dirlen - 1] == '/')
		slicepath[dirlen - 1] = 0;

	printf("Slicing image into %s...\n", slicepath);
	mkdir(slicepath, S_IRWXU | S_IRWXG | S_IRWXO);
	clock_t start = clock();

	uint8_t zoomlevels = (uint8_t)ceil(log2((double)img->height / TILESIZE));

	image *zimg = img;
	for (int8_t z = zoomlevels; z >= 0; z--)
	{
		printf("Zoom level %d (1:%d): %d x %d pixels, %d x %d tiles\n", z,
				(uint8_t)pow(2, zoomlevels - z), zimg->width, zimg->height,
				(zimg->width + TILESIZE - 1) / TILESIZE, (zimg->height + TILESIZE - 1) / TILESIZE);

		char zoompath[255];
		sprintf(zoompath, "%s/zoom%d", slicepath, z);

		// clear all files in zoom directory
		printf("Clearing/creating directory %s\n", zoompath);
		nftw(zoompath, rm_file, 1, FTW_DEPTH);
		mkdir(zoompath, S_IRWXU | S_IRWXG | S_IRWXO);

		// slice
		slice_image(zimg, TILESIZE, zoompath);

		// replace image with 50% scaled version
		image *nextimg;
		if (z > 0)
		{
			printf("Scaling image by half\n");
			nextimg = scale_image_half(zimg);
		}
		if (z < zoomlevels) free(zimg);
		zimg = nextimg;
	}

	printf("Total save time: %f seconds\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}


int main(int argc, char **argv)
{
	char *inpath = NULL;
	char *outpath = NULL;
	char *slicepath = NULL;
	static options opts =
	{
		.limits    = NULL,
		.ylimits   = NULL,
		.texpath   = "resources/textures.csv",
		.shapepath = "resources/shapes.csv",
		.biomepath = "resources/biomes.csv",
	};
	uint8_t rotateint;
	int32_t fc = 0;
	int32_t f1, f2, f3, fx, fy, fz;
	int32_t tc = 0;
	int32_t t1, t2, t3, tx, ty, tz;

	// flush output on newlines
	setvbuf(stdout, NULL, _IOLBF, 0);

	static struct option long_options[] =
	{
		{"isometric", no_argument, (int*)&opts.isometric, 1},
		{"dark",      no_argument, (int*)&opts.dark,      1},
		{"shadows",   no_argument, (int*)&opts.shadows,   1},
		{"biomes",    no_argument, (int*)&opts.biomes,    1},
		{"tiny",      no_argument, (int*)&opts.tiny,      1},
		{"nether",    no_argument, (int*)&opts.nether,    1},
		{"end",       no_argument, (int*)&opts.end,       1},
		{"rotate",    required_argument, 0, 'r'},
		{"world",     required_argument, 0, 'w'},
		{"output",    required_argument, 0, 'o'},
		{"googlemap", required_argument, 0, 'g'},
		{"from",      required_argument, 0, 'F'},
		{"to",        required_argument, 0, 'T'},
		{0, 0, 0, 0}
	};

	int c;
	while (1)
	{
		int option_index = 2;
		c = getopt_long(argc, argv, "-idsbtner:w:o:g:F:T:", long_options, &option_index);
		if (c == -1) break;

		switch (c)
		{
		case 0: // flag set with long option
		case '?': // unknown option
			break;

		case 'i':
			opts.isometric = 1;
			break;

		case 'd':
			opts.dark = 1;
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

		case 'n':
			opts.nether = 1;
			break;

		case 'e':
			opts.end = 1;
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

		case 'g':
			slicepath = optarg;
			break;

		case 'F':
			fc = sscanf(optarg, "%d,%d,%d", &f1, &f2, &f3);
			if (!fc) fprintf(stderr, "Invalid 'from' coordinates: %s\n", optarg);
			break;

		case 'T':
			tc = sscanf(optarg, "%d,%d,%d", &t1, &t2, &t3);
			if (!tc) fprintf(stderr, "Invalid 'to' coordinates: %s\n", optarg);
			break;

		default:
			abort();
		}
	}

	if (inpath == NULL)
	{
		fprintf(stderr, "Please specify a region directory with -w.\n");
		return 1;
	}

	// default to single-image mode
	if (outpath == NULL && slicepath == NULL) outpath = "map.png";

	if (fc != tc)
		fprintf(stderr, "'From' and 'to' coordinates must be in the same format (X,Z or X,Y,Z).\n");
	else if (fc == 1)
	{
		uint8_t ylimits[2] =
		{
			MAX(0, MIN(f1, t1)),
			MIN(MAX_HEIGHT, MAX(f1, t1)),
		};
		opts.ylimits = ylimits;
	}
	else if (fc > 1)
	{
		fx = f1;
		tx = t1;
		if (fc == 3)
		{
			fz = f3;
			tz = t3;

			uint8_t ylimits[2] =
			{
				MAX(0, MIN(f2, t2)),
				MIN(MAX_HEIGHT, MAX(f2, t2)),
			};
			opts.ylimits = ylimits;
		}
		else
		{
			fz = f2;
			tz = t2;
		}

		int32_t limits[4] =
		{
			MIN(fz, tz),
			MAX(fx, tx),
			MAX(fz, tz),
			MIN(fx, tx),
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

		if (opts.dark) printf("Dark mode is on\n");
		else if (opts.isometric && opts.shadows) printf("Daytime shadows are on\n");

		if (opts.biomes) printf("Biomes are on\n");

		if (opts.nether) printf("Rendering nether dimension\n");
		else if (opts.end) printf("Rendering end dimension\n");
	}

	image *img = create_world_map(inpath, &opts);
	if (img == NULL) return 1;

	if (outpath != NULL) save_world_map_image(img, outpath);
	if (slicepath != NULL) save_world_map_slices(img, slicepath);

	free_image(img);

	return 0;
}
