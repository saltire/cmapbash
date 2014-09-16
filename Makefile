sources = ${wildcard *.c}
objects = ${sources:%.c=%.o} \
	cNBT/buffer.o cNBT/nbt_loading.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o \
	lodepng/lodepng.o


all: cmapbash

debug: CFLAGS = -g
debug: cmapbash

cmapbash: $(objects)
	gcc $(CFLAGS) $(objects) -lz -o cmapbash

%.o: %.c
	gcc $(CFLAGS) -std=c99 $< -c -IcNBT -Ilodepng

cNBT/%.o: cNBT/%.c
	gcc -std=c99 $< -c -o $@

lodepng/%.o: lodepng/%.c
	gcc $< -c -o $@

clean:
	rm -f cmapbash *.o */*.o
