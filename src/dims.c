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


unsigned int get_offset(const unsigned int y, const unsigned int rx, const unsigned int rz,
		const unsigned int length, const unsigned char rotate)
{
	int x, z;
	int max = length - 1;
	switch(rotate) {
	case 0:
		x = rx;
		z = rz;
		break;
	case 1:
		x = rz;
		z = max - rx;
		break;
	case 2:
		x = max - rx;
		z = max - rz;
		break;
	case 3:
		x = max - rz;
		z = rx;
		break;
	}
	return (y * length + z) * length + x;
}
