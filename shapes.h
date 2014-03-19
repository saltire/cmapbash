#include "textures.h"


unsigned char shapes[4][ISO_BLOCK_WIDTH * ISO_BLOCK_HEIGHT] = {
		// 0: solid
		{
				1,1,1,1,
				1,1,1,1,
				1,1,1,1,
				1,1,1,1
		},

		// 1: flat
		{
				0,0,0,0,
				0,0,0,0,
				0,0,0,0,
				1,1,1,1
		},

		// 2: small square
		{
				0,0,0,0,
				0,0,0,0,
				0,1,1,0,
				0,1,1,0
		},

		// 3: grass
		{
				0,0,0,0,
				0,0,1,0,
				1,0,1,1,
				0,1,1,0
		}
};
