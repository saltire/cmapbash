#include "image.h"
#include "textures.h"


typedef struct world {
	const char* dir;
	int rcount, rxmin, rxmax, rzmin, rzmax, rxsize, rzsize;
	unsigned char* regionmap;
} world;


image render_world_map(world world, const texture* textures,
		const char night, const char isometric, const char rotate);
void save_world_map(const char* worlddir, const char* imagefile, const texture* textures,
		const char night, const char isometric, const char rotate);
