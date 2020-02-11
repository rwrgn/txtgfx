#include "example.h"

int main(void) {
	int i, j;

	// Seed rng
	srand(time(0));

	// Initialize text mode
	initTextMode();
	

	// Screen 1
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 16; j++) {
			fillRectToBlockBuffer(j * 5, i * 5, 5, 5, (j + i) % 8);
		}
	}
	drawScreenFromBlockBuffer();
	printColorStringToScreen("Text Mode Initialized. Drawing some rectangles.", 0, 0, 7);
	promptForKey();
	// s1

	// Screen 2
	showCursor(false);
	setBlinking(false);

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 16; j++) {
			fillRectToBlockBuffer(j * 5, i * 5, 5, 5, (j + i) % 16);
		}
	}
	drawScreenFromBlockBuffer();
	printColorStringToScreen("Cursor blinking disabled, color blinking disabled", 0, 0, 7);
	promptForKey();
	// s2

	// Screen 3
	randomizeAllColors();
	drawScreenFromBlockBuffer();
	printColorStringToScreen("Palette randomized.", 0, 0, 7);
	promptForKey();
	// s3

	// Screen 4
	setColor(0, 0, 0, 0);
	for (i = 1; i < 16; i++) {
		setColor(i, i, i * 2, (i + 1) * 4 - 1);
	}
	drawScreenFromBlockBuffer();
	printColorStringToScreen("Palette set to blue hues.", 0, 0, 7);
	promptForKey();
	// s4

	// Screen 5
	clrScr();
	clrBlockColorBuffer(0);

	for (i = 0; i < 20; i++) {
		j = rand() % 3;

		if (j == 0) {
			strokeRectToBlockBuffer(rand() % (COLS - 8), rand() % (ROWS * 2 - 8), rand() % 8, rand() % 8, rand() % 15 + 1);
		}
		else if (j == 1) {
			triangleToBlockBuffer(rand() % COLS, rand() % (ROWS * 2), rand() % COLS, rand() % (ROWS * 2), rand() % COLS, rand() % (ROWS * 2), rand() % 15 + 1);
		}
		else {
			strokeCircleToBlockBuffer(rand() % (COLS - 8), rand() % (ROWS * 2 - 8), rand() % 8, rand() % 15 + 1);
		}

	}
	drawScreenFromBlockBuffer();
	printColorStringToScreen("Filling the screen with random shapes.", 0, 0, 7);
	promptForKey();
	// s5

	// Screen 6
	clrScr();
	clrBlockColorBuffer(0);
	printLargeStringToBuffer(4, 4, "PRINTING\nLARGE\nTEXT.", 10);
	drawScreenFromBlockBuffer();
	promptForKey();
	// s6

	// Screen 7
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 16; j++) {
			fillRectToBlockBuffer(j * 5, i * 5, 5, 5, (j + i) % 2 + 2);
		}
	}
	printLargeStringToBuffer(4, 4, "SCROLLING...", 10);

	while (!kbhit()) {
		
		shiftBlockBuffer(-1, 0);
		delay(50);
		drawScreenFromBlockBuffer();
		printColorStringToScreen("Press any key to continue.", 54, 24, 7);

	} getch();
	// s7

	// Screen 8
	rotateBlockBuffer(.2);
	drawScreenFromBlockBuffer();
	printColorStringToScreen("Rotating.", 0, 0, 7);
	printColorStringToScreen("Press any key to quit.", 58, 24, 7);
	while (!kbhit()) {
		;
	} getch();
	// s8

	// Set palette and character set to default values before exiting.
	initTextMode();

	return 0;

}

void promptForKey(void) {
	printColorStringToScreen("Press any key to continue.", 54, 24, 7);
	while (!kbhit()) {
		;
	} getch();
}