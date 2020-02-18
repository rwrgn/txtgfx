# txtgfx
txtgfx.c, txtgfx.h, palettes.h, palettes.cpp
80x25 Text Mode graphics routines for DOS (32-bit protected mode)

v0.002b; February 2020

Mainly provides functionality to draw graphics primitives with code page 437 block graphics (i.e. characters 219, 220 and 223), including functions to print text with ~3x5 character sizes using said block characters.

Also includes functions for palette and character set manipulation, and the functionality to load .bin format ANSI graphics files.

Additional palette functions (e.g. saving and loading) declared in palettes.h and defined in palettes.cpp naturally require C++.

Please see example.c for examples.

Works as is with Open Watcom 1.9 and 2.0 32-bit compilers (C/C++).

N.B.: The purpose of this project is mostly to educate myself on programming text mode graphics in DOS. Most of the code is mainly optimized (if at all) for speed at the expense of readable code.

