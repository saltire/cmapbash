sources = ${wildcard *.cpp}
objects = ${sources:%.cpp=%.o} \
	cNBT/buffer.o cNBT/nbt_loading.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o \
	lodepng/lodepng.o


cNBT/%.o : cNBT/%.c
	gcc -std=c99 $< -c -o $@

lodepng/%.o : lodepng/%.cpp
	g++ $< -c -o $@

%.o : %.cpp
	g++ $< -c -I cNBT/ -I lodepng/

cmapbash : $(objects)
	g++ $(objects) -lz -o cmapbash


clean :
	rm -f cmapbash *.o */*.o
