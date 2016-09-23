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


#ifndef DATA_H_
#define DATA_H_


#include <stdbool.h>
#include <stdint.h>

#include "nbt.h"


// world dimensions

#define CHUNK_BLOCK_BITS 4
#define REGION_CHUNK_BITS 5
#define REGION_BLOCK_BITS (REGION_CHUNK_BITS + CHUNK_BLOCK_BITS)

#define CHUNK_BLOCK_LENGTH (1 << CHUNK_BLOCK_BITS)
#define CHUNK_BLOCK_AREA (CHUNK_BLOCK_LENGTH * CHUNK_BLOCK_LENGTH)
#define SECTION_BLOCK_HEIGHT 16
#define CHUNK_SECTION_HEIGHT 16
#define CHUNK_BLOCK_HEIGHT (SECTION_BLOCK_HEIGHT * CHUNK_SECTION_HEIGHT)
#define SECTION_BLOCK_VOLUME (SECTION_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)
#define CHUNK_BLOCK_VOLUME (CHUNK_BLOCK_HEIGHT * CHUNK_BLOCK_AREA)

#define REGION_CHUNK_LENGTH (1 << REGION_CHUNK_BITS)
#define REGION_BLOCK_LENGTH (1 << REGION_BLOCK_BITS)
#define REGION_CHUNK_AREA (REGION_CHUNK_LENGTH * REGION_CHUNK_LENGTH)
#define REGION_BLOCK_AREA (REGION_CHUNK_AREA * CHUNK_BLOCK_AREA)


// maximum values

#define MAX_CHUNK_BLOCK (CHUNK_BLOCK_LENGTH - 1)
#define MAX_REGION_CHUNK (REGION_CHUNK_LENGTH - 1)
#define MAX_REGION_BLOCK (REGION_BLOCK_LENGTH - 1)
#define MAX_HEIGHT (CHUNK_BLOCK_HEIGHT - 1)


// path lengths

#define WORLDDIR_PATH_MAXLEN 255
#define REGIONDIR_PATH_MAXLEN (WORLDDIR_PATH_MAXLEN + 13)
#define REGION_COORD_MAXLEN 8
#define REGIONFILE_PATH_MAXLEN (REGIONDIR_PATH_MAXLEN + REGION_COORD_MAXLEN * 2 + 8)


// region file constants

#define SECTOR_BYTES 4096
#define OFFSET_BYTES 4
#define LENGTH_BYTES 4


// absolute directions relative to block data
typedef enum
{
	NORTH,
	EAST,
	SOUTH,
	WEST
}
dirs;

// directions relative to the rotated map image
typedef enum
{
	TOP,
	RIGHT,
	BOTTOM,
	LEFT
}
edges;

// directions relative to rotated isometric blocks
typedef enum
{
	TOP_RIGHT,
	BOTTOM_RIGHT,
	BOTTOM_LEFT,
	TOP_LEFT
}
iso_edges;

// binary data for a chunk of blocks
typedef struct chunk_data
{
	uint8_t *blimits; // pointer to an array of absolute min/max x/z block coords for this chunk
	uint8_t *bids, *bdata, *blight, *slight, *biomes;
	                  // pointers to byte data arrays for this chunk
	uint8_t *nbids[4], *nbdata[4], *nblight[4], *nslight[4];
	                  // arrays of pointers to byte data arrays for each rotated neighbouring chunk
}
chunk_data;

// booleans indicating which types of chunk data to load for this render
typedef struct chunk_flags
{
	bool bids, bdata, blight, slight, biomes; // whether to load each type of chunk data
}
chunk_flags;

// a region containing a number of chunks
typedef struct region
{
	int32_t x, z;                         // absolute world-level coords of this region
	char path[REGIONFILE_PATH_MAXLEN];    // path to the region file
	uint32_t offsets[REGION_CHUNK_AREA];  // byte offset values for each chunk in the region file
	uint16_t *blimits;                    // pointer to an array of absolute min/max
	                                      //   x/z block coords for this region
	uint8_t *cblimits[REGION_CHUNK_AREA]; // pointers to arrays of absolute min/max
	                                      //   x/z block coords for each chunk in this region
	FILE *file;                           // handle for the region file
}
region;

// a world containing a number of regions
typedef struct worldinfo
{
	char regiondir[REGIONDIR_PATH_MAXLEN]; // path to the region directory
	uint32_t rcount;                       // the number of existing region files in the directory
	uint32_t rrxsize, rrzsize;             // rotated width and height of the world in regions
	uint32_t rrxmax, rrzmax;               // rotated maximum x/z region coords
	                                       //   (i.e. width and height minus 1)
	uint8_t rotate;                        // the number of times to rotate the map 90 degrees
	region **regionmap;                    // pointer to an array of region structs,
	                                       //   indexed by rotated offset
}
worldinfo;


/* get a block's absolute chunk-level 3D offset from rotated coordinates
 *   rbx, rbz: the block's rotated chunk-level x/z coords
 *   y:        the block's y coord
 *   rotate:   the rotate value
 */
uint16_t get_block_offset(const uint8_t rbx, const uint8_t rbz, const uint8_t y,
		const uint8_t rotate);

/* get a chunk's absolute region-level offset from rotated coordinates
 *   rcx, rcz: the chunk's rotated region-level x/z coords
 *   rotate:   the rotate value
 */
uint16_t get_chunk_offset(const uint8_t rcx, const uint8_t rcz, const uint8_t rotate);

/* get the data values for the 4 neighbouring blocks
 *   nvalues:  an output array of 4 data values
 *   cdata:    chunk data for the current chunk
 *   ncdata:   chunk data for the 4 neighbouring chunks, in case we're on an edge
 *   rbx, rbz: the block's rotated chunk-level x/z coords
 *   y:        the block's y coord
 *   rotate:   the rotate value
 *   defval:   a default value for nonexistent blocks
 */
void get_neighbour_values(uint8_t nvalues[4], uint8_t *cdata, uint8_t *ncdata[4], uint8_t defval,
		const uint8_t rbx, const uint8_t rbz, const uint8_t y, const uint8_t rotate);

/* generate a chunk data struct from raw chunk data in the region file
 *   cdata:    pointer to the binary data
 *   length:   length of the binary data
 *   flags:    pointer to a struct indicating which byte arrays to read from the NBT node
 *   cblimits: pointer to an array of absolute min/max x/z block coords for this chunk
 *   ylimits:  pointer to an array of min/max y coords
 */
chunk_data *parse_chunk_nbt(const uint8_t *cdata, const uint32_t length, const chunk_flags *flags,
		uint8_t *cblimits, const uint8_t *ylimits);

/* locate and read raw chunk data from the region file and return a chunk data struct
 *   reg:      pointer to the region struct
 *   rcx, rcz: the chunk's rotated region-level x/z coords
 *   rotate:   the rotate value
 *   flags:    pointer to a struct indicating which byte arrays to read from the NBT node
 *   ylimits:  pointer to an array of min/max y coords
 */
chunk_data *read_chunk(const region *reg, const uint8_t rcx, const uint8_t rcz,
		const uint8_t rotate, const chunk_flags *flags, const uint8_t *ylimits);

/* free the memory used for a chunk data struct
 *   chunk: pointer to the chunk data struct
 */
void free_chunk(chunk_data *chunk);

/* check whether a chunk's data is present in a region file
 *   reg:      pointer to the region struct
 *   rcx, rcz: the chunk's rotated region-level coords
 *   rotate:   the rotate value
 */
bool chunk_exists(const region *reg, const uint8_t rcx, const uint8_t rcz, const uint8_t rotate);

/* open the file for a region and return the file handle
 *   reg: pointer to the region struct
 */
FILE *open_region_file(region *reg);

/* close the file handle for a region
 *   reg: pointer to the region struct
 */
void close_region_file(region *reg);

/* generate a region struct given a region directory and coordinates (or NULL if nonexistent)
 *   regiondir: path to the region directory
 *   rx, rz:    absolute world-level coords for the region
 *   rblimits:  pointer to an array of absolute min/max x/z block coords for this region
 */
region *read_region(const char *regiondir, const int32_t rx, const int32_t rz,
		const uint16_t *rblimits);

/* free the memory used for a region struct
 *   reg: pointer to the region struct
 */
void free_region(region *reg);

/* get a stored region struct from its rotated world-relative coords
 *   world:    pointer to the world struct
 *   rrx, rrz: the region's rotated world-relative x/z coords
 */
region *get_region_from_coords(const worldinfo *world, const uint32_t rrx, const uint32_t rrz);

/* generate a world struct given a world directory
 *   worldpath: path to the world directory
 *   rotate:    the rotate value to use when rendering this world
 *   wblimits:  pointer to an array of absolute world-level min/max x/z block coords
 *   hell:      whether to render hell dimension (overrides end)
 *   end:       whether to render end dimension
 */
worldinfo *measure_world(char *worldpath, const uint8_t rotate, const int32_t *wblimits,
	const bool hell, const bool end);

/* free the memory used for a world struct
 *   world: pointer to the world struct
 */
void free_world(worldinfo *world);


#endif
