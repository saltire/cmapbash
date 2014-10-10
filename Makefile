CC     ?= gcc
CFLAGS  = -std=c99


datasrc = $(wildcard src/data/*.c) \
	$(addprefix src/data/cNBT/, buffer.c nbt_loading.c nbt_parsing.c nbt_treeops.c nbt_util.c)
dataobj = $(datasrc:src/%.c=obj/%.o)
	
mapsrc = $(wildcard src/map/*.c) \
	src/map/lodepng/lodepng.c
mapobj = $(mapsrc:src/%.c=obj/%.o)

dir_guard = @mkdir -p $(@D)


.PHONY: clean debug data


bin/cmapbash: $(mapobj) $(dataobj)
	$(dir_guard)
	$(CC) $(CFLAGS) $(mapobj) $(dataobj) -lz -o bin/cmapbash
	@cp -r resources bin

obj/%.o: src/%.c
	$(dir_guard)
	$(CC) $(CFLAGS) $< -c -o $@ -Isrc/map/lodepng -Isrc/data -Isrc/data/cNBT


clean:
	@rm -rf bin obj
	
debug: CFLAGS += -g
debug: bin/cmapbash
