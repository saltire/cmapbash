objects = cNBT/buffer.o cNBT/nbt_loading.o cNBT/nbt_parsing.o \
	cNBT/nbt_treeops.o cNBT/nbt_util.o


cmapbash : cmapbash.o $(objects)
	g++ cmapbash.o $(objects) -lz -o cmapbash

cmapbash.o :
	g++ cmapbash.cpp -c -I cNBT/
