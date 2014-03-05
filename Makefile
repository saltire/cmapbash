objects = cNBT/buffer.o cNBT/nbt_loading.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o \
	lodepng/lodepng.o
	


cmapbash : cmapbash.o chunk.o $(objects)
	g++ cmapbash.o chunk.o $(objects) -lz -o cmapbash

cmapbash.o : cmapbash.cpp
	g++ cmapbash.cpp -c -I cNBT/ -I lodepng/

chunk.o : chunk.cpp
	g++ chunk.cpp -c -I cNBT/

clean :
	rm -f cmapbash *.o cNBT/*.o lodepng/*.o
