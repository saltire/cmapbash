CC     ?= gcc
CFLAGS  = -std=c99


sources = $(wildcard src/*.c) \
	$(addprefix src/cNBT/, buffer.c nbt_loading.c nbt_parsing.c nbt_treeops.c nbt_util.c) \
	src/lodepng/lodepng.c
objects = $(sources:src/%.c=obj/%.o)
deps = cNBT lodepng

dir_guard = @mkdir -p $(@D)


.PHONY: clean debug linux windows


bin/cmapbash: $(objects)
	$(dir_guard)
	$(CC) $(CFLAGS) $(objects) -lz -o bin/cmapbash
	@cp -r resources bin

obj/%.o: src/%.c
	$(dir_guard)
	$(CC) $(CFLAGS) $< -c -o $@ $(foreach dep,$(deps),-Isrc/$(dep))

$(foreach dep,$(deps),obj/$(dep)/%.o): $(foreach dep,$(deps),obj/$(dep)/%.c)
	$(dir_guard)
	$(CC) $(CFLAGS) $< -c -o $@
	

clean:
	@rm -rf bin obj
	
debug: CFLAGS += -g
debug: bin/cmapbash
