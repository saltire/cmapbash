
#include "list.h"
#include "nbt.h"

#include "chunk.h"

#include <errno.h>
#include <iostream>

using namespace std;

#define BLOCKS 16
#define SECHEIGHT 16
#define SECTIONS 16
#define CHUNKSIZE BLOCKS * BLOCKS
#define SECSIZE SECHEIGHT * CHUNKSIZE


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

	int32_t* hmap = get_chunk_heightmap(chunk);
	for (int i = 0; i < CHUNKSIZE; i++) {
		cout << hmap[i] << " ";
	}
	free(hmap);

	nbt_free(chunk);

	return 0;
}

