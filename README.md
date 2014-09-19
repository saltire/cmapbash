cmapbash
========

A Minecraft map renderer written in C.

Uses the following libraries:
- cNBT - https://github.com/FliPPeh/cNBT
- LodePNG - http://lodev.org/lodepng

Supports orthographic and isometric rendering.

Options so far:
- `-i` - Isometric mode.
- `-n` - Night mode.
- `-s` - Render daytime shadows in isometric mode (experimental, may be buggy).
- `-t` - Tiny mode. Renders a quick minimap of all existing chunks.
- `-o [filename]` - The image file to save to. Defaults to `map.png`.
- `-r [#]` - Rotate the map `#` x 90 degrees clockwise.
By default, north is at the top in orthographic mode,
and northwest is at the top in isometric mode.

This happens to be my first C project.
