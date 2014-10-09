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


#ifndef CHUNKDATA_H_
#define CHUNKDATA_H_


#include "nbt.h"


typedef struct chunk_data
{
	nbt_node *nbt;
	unsigned int *blimits;
	unsigned char *bids, *bdata, *blight, *slight;
	unsigned char *nbids[4], *nbdata[4], *nblight[4], *nslight[4];
}
chunk_data;


unsigned char *get_chunk_data(chunk_data *chunk, unsigned char *tag, const char half,
		const char defval, const unsigned int *ylimits);


#endif
