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
- `-s` - Render sunlight shadows in isometric mode.
- `-t` - Tiny mode. Renders a quick minimap of all existing chunks.
- `-w <directory>` - The path to the Minecraft world folder (required).
- `-o <filename>` - The path at which to save a single image.
  Defaults to `map.png`, unless `-g` is specified, in which case defaults to none. 
- `-g <directory>` - The directory in which to save a set of tiles,
  suitable for use with Google Maps.
  This will create subfolders for a number of zoom levels, depending on the map's size.
- `-r <#>` - Rotate the map `#` x 90 degrees clockwise.
  By default, north is at the top in orthographic mode,
  and northwest is at the top in isometric mode.
- `-F <Y> -T <Y>` - Render only the vertical slice from one height to the other.
- `-F <X>,<Z> -T <X>,<Z>` - Render only the rectangular area from one corner to the other.
- `-F <X>,<Y>,<Z> -T <X>,<Y>,<Z>` - Render only the cuboid from one set of coordinates to the other.

This happens to be my first C project.
