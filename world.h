#include "image.h"
#include "textures.h"


typedef struct world {
	const char* dir;
	int rcount, rxmin, rxmax, rzmin, rzmax, rxsize, rzsize;
	unsigned char* regionmap;
} world;


image render_world_blockmap(world world, const texture* textures, const char night,
		const char rotate);
image render_world_iso_blockmap(world world, const texture* textures, const char night,
		const char rotate);

void save_world_blockmap(const char* worlddir, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate);
