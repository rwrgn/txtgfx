#include "palettes.h"

/**
 * spd < 1.
 */
void fadeToPaletteSlow(Palette* palette, float spd) {
	static float i = 0.0;

	i += spd;
	if (i >= 1) {
		fadeToPalette(palette);
		i = 0;
	}
}

void fadeToPalette(Palette* palette) {
	int i;
	for (i = 0; i < 16; i++) {
		fadeToColor(i, (*palette).r[i], (*palette).g[i], (*palette).b[i]);
	}
}

void loadPalette(Palette* palette) {
	int i;
	for (i = 0; i < 16; i++) {
		setColor(i, (*palette).r[i], (*palette).g[i], (*palette).b[i]);
	}
}

void savePalette(Palette* palette) {
	int i;
	int r, g, b;
	for (i = 0; i < 16; i++) {
		getColor(i, &r, &g, &b);
		(*palette).setColor(i, r, g, b);
	}
}
