#include "image.h"
#include "textures.h"


typedef struct world {
	const char* dir;
	int rcount, rxmin, rxmax, rzmin, rzmax, rxsize, rzsize;
	int* regions;
} world;


image render_world_blockmap(world world, const texture* textures, const char night);
image render_world_iso_blockmap(world world, const texture* textures, const char night);

void save_world_blockmap(const char* worlddir, const char* imagefile, const texture* textures,
		const char night, const char isometric);
