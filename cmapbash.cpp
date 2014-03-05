#include "nbt.h"

#include "lodepng.h"

#include "chunk.h"
#include "render.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>

using namespace std;

#define BLOCKS 16
#define CHUNKSIZE BLOCKS * BLOCKS


int main(int argc, char **argv)
{
	if (argc < 2)
	{
		cout << "Please specify an NBT file to parse." << endl;
		return 0;
	}

	cout << "Reading file " << argv[1] << endl;
	nbt_node* chunk = nbt_parse_path(argv[1]);
	if (errno != NBT_OK)
	{
		cout << "Parsing error!" << endl;
		return 0;
	}

	//cout << nbt_dump_ascii(chunk);

	unsigned char* heightmap = get_chunk_heightmap(chunk);
	nbt_free(chunk);

	render_greyscale("chunk.png", heightmap, BLOCKS, BLOCKS);
	free(heightmap);

	return 0;
}

