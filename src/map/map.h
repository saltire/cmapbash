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


#ifndef MAP_H
#define MAP_H


#include <stdbool.h>
#include <stdint.h>

#include "data.h"
#include "image.h"
#include "textures.h"


// configurable render options

#define HILIGHT_AMOUNT 0.125 // amount to lighten isometric block highlights
#define SHADOW_AMOUNT -0.125 // amount to darken isometric block shadows
#define HSHADE_HEIGHT 0.3 // height below which to add shadows
#define HSHADE_AMOUNT 0.7 // amount of shadow to add
#define NIGHT_AMBIENCE 0.2 // base light level for night renders

#define HSHADE_BLOCK_HEIGHT (HSHADE_HEIGHT * MAX_HEIGHT)


// data constants

#define LIGHT_LEVELS 16
#define MAX_LIGHT (LIGHT_LEVELS - 1)


// min/max macros

#define MIN(x, y) ({ \
	__typeof__(x) _x = (x); \
	__typeof__(y) _y = (y); \
	_x < _y ? _x : _y; })

#define MAX(x, y) ({ \
	__typeof__(x) _x = (x); \
	__typeof__(y) _y = (y); \
	_x > _y ? _x : _y; })


// options for rendering the map
typedef struct options
{
	bool isometric,   // whether to render an isometric (true) or orthographic (false) map.
		night,        // whether to render in night mode
		shadows,      // whether to include sunlight/moonlight shadows
		biomes,       // whether to use biome colours
		tiny;         // whether to render a minimap where each existing chunk is a white pixel
	uint8_t rotate;   // how many times to rotate the map 90 degrees clockwise
	int32_t *limits;  // pointer to an array of absolute min/max x/z block coords to crop to
	                  //   (ymin, xmax, ymax, xmin)
	uint8_t *ylimits; // pointer to an array of absolute min/max y coords to crop to
	char *texpath,    // path to a block texture/colour CSV file
		*shapepath,   // path to an isometric blocktype shape file
		*biomepath;   // path to a biome colour CSV file
}
options;


/* render a single column of blocks to an isometric map
 *   img:      pointer to the map's image struct
 *   px, py:   pixel coords of the top left corner of the column
 *   tex:      pointer to the texture struct
 *   chunk:    pointer to the chunk data struct
 *   rbx, rbz: the column's rotated chunk-level x/z coords
 *   opts:     pointer to the render options struct
 */
void render_iso_column(image *img, const int32_t px, const int32_t py, const textures *tex,
		chunk_data *chunk, const uint8_t rbx, const uint8_t rbz, const options *opts);

/* render a single column of blocks to an orthographic map
 *   img:      pointer to the map's image struct
 *   px, py:   pixel coords of the top left corner of the column
 *   tex:      pointer to the texture struct
 *   chunk:    pointer to the chunk data struct
 *   rbx, rbz: the column's rotated chunk-level x/z coords
 *   opts:     pointer to the render options struct
 */
void render_ortho_column(image *img, const int32_t px, const int32_t py, const textures *tex,
		chunk_data *chunk, const uint32_t rbx, const uint32_t rbz, const options *opts);

/* get the width of empty space that would be on each edge of the rendered map for this region
 *   margins:   pointer to the output array of pixel values for each edge
 *   reg:       pointer to the region struct
 *   rotate:    the rotate value
 *   isometric: whether we are rendering an isometric or orthographic map
 */
void get_region_margins(uint32_t *rmargins, region *reg, const uint8_t rotate,
		const bool isometric);

/* render a single white pixel for each existing chunk in the region onto the map
 *   img:      pointer to the image struct
 *   rpx, rpy: pixel coords of the top left corner of this region
 *   reg:      pointer to the region struct
 *   opts:     pointer to the render options struct
 */
void render_tiny_region_map(image *img, const int32_t rpx, const int32_t rpy, region *reg,
		const options *opts);

/* render a full region onto the map
 *   img:      pointer to the image struct
 *   rpx, rpy: pixel coords of the top left corner of this region
 *   reg:      pointer to the region struct
 *   nregions: array of pointers to the rotated neighbouring region structs
 *   tex:      pointer to the texture struct
 *   opts:     pointer to the render options struct
 */
void render_region_map(image *img, const int32_t rpx, const int32_t rpy, region *reg,
		region *nregions[4], const textures *tex, const options *opts);

/* render the full world onto the map
 *   img:      pointer to the image struct
 *   wpx, wpy: pixel coords of the top left corner of the world (should be zero or negative)
 *   world:    pointer to the world struct
 *   opts:     pointer to the render options struct
 */
void render_world_map(image *img, int32_t wpx, int32_t wpy, const worldinfo *world,
		const options *opts);

/* render a map from a world directory and return a pointer to an image struct
 *   worldpath: path to the world directory
 *   imgpath:   path to the output image file
 *   opts:      pointer to a render options struct
 */
image *create_world_map(char *worldpath, const options *opts);


#endif
