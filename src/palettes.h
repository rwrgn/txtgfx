#ifndef _PALETTES_H
#define _PALETTES_H

#include "txtgfx.h"

class Palette {
public:
	int r[16];
	int g[16];
	int b[16];

	Palette() {
		for (int i = 0; i < 16; i++) {
			r[i] = 0;
			g[i] = 0;
			b[i] = 0;
		}
	}

	void setColor(int c, int rr, int gg, int bb) {
		r[c] = rr;
		g[c] = gg;
		b[c] = bb;
	}
};

void loadPalette(Palette* palette);
void savePalette(Palette* palette);
void fadeToPalette(Palette* palette);
void fadeToPaletteSlow(Palette* palette, float spd);

#endif
