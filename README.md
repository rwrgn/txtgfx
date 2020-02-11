# txtgfx
80x25 Text Mode graphics routines for DOS (32-bit protected mode)

v0.002; February 2020

Mainly provides functionality to draw graphics primitives with code page 437 block graphics (i.e. characters 219, 220 and 223), including functions to print text with ~3x5 character sizes using said block characters.

Also includes functions for palette and character set manipulation, and the functionality to load .bin format ANSI graphics files.

Please see example.c for examples.

Works as is with Open Watcom 1.9 and 2.0 32-bit compilers.

N.B.: The purpose of this project is mostly to educate myself on programming text mode graphics in DOS. Most of the code is mainly optimized (if at all) for speed at the expense of readable code.

