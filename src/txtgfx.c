/**
 * txtgfx.c, txtgfx.h, palettes.h, palettes.cpp
 * 80x25 Text Mode graphics routines for DOS (32-bit protected mode)
 * v0.002b; February 2020
 * 
 * Mainly provides functionality to draw graphics primitives with
 * code page 437 block graphics (i.e. characters 219, 220 and 223),
 * including functions to print text with ~3x5 character sizes using
 * said block characters.
 *
 * Also includes functions for palette and character set manipulation,
 * and the functionality to load .bin format ANSI graphics files.
 *
 * Additional palette functions (e.g. saving and loading) declared in
 * palettes.h and defined in palettes.cpp naturally require C++.
 *
 * Please see example.c for examples.
 *
 * Works as is with Open Watcom 1.9 and 2.0 32-bit compilers (C/C++).
 *
 * N.B.: The purpose of this project is mostly to educate myself on
 * programming text mode graphics in DOS. Most of the code is mainly
 * optimized (if at all) for speed at the expense of readable code.
 *
 * [The rest of the comments are in Finnish]
 */

#include "txtgfx.h"

/**
 * A number of global buffers, with hopefully self-explanatory names.
 */
char screenCharBuffer[ROWS][COLS];
char screenCharBackupBuffer[ROWS][COLS];
char screenColorBuffer[ROWS][COLS];
char screenColorBackupBuffer[ROWS][COLS];
char blockColorBuffer[2 * ROWS][COLS];
char blockColorBackupBuffer[2 * ROWS][COLS];
char transformBuffer[2 * ROWS][COLS];

// Bufferit bin-kuville.
char imageBuffer[2 * ROWS * COLS];

/**
 * Muuttaa v‰ri‰ colorNumber.
 */
void setColor(int colorNumber, int r, int g, int b) {
	// Katso:
	// http://www.techhelpmanual.com/144-int_10h_1010h__set_one_dac_color_register.html

	union REGS in, out;

	// Huom.! Kirkkaat v‰rit sijaitsevat jostain syyst‰ v‰lill‰ 56-63?!!
	// T‰m‰n vuoksi seuraava korjaus:

	if (colorNumber > 7 && colorNumber < 16) {
		colorNumber += 48;
	}

	// Ruskea sijaitsee kohdassa 20?!
	else if (colorNumber == 6) {
		colorNumber = 20;
	}

	// Huom.: V‰rit ovat rekistereiss‰ muodossa GBR. Rekisteri on 1010h.
	in.w.ax = 0x1010;
	in.w.bx = colorNumber;
	in.h.dh = r;
	in.h.ch = g;
	in.h.cl = b;
	int386(0x10, &in, &out);
}

/**
 * Hakee v‰rin colorNumber rgb-arvot paletista ja asettaa ne muuttujien
 * osoitteisiin &r, &g ja &b. 
 */
void getColor(int colorNumber, int* r, int* g, int* b) {

	char rstr[2];
	char gstr[2];
	char bstr[2];

	union REGS in, out;

	// Seiskaa suuremmat v‰rit sijaitsevatkin loppup‰‰ss‰.
	if (colorNumber > 7 && colorNumber < 16) {
		colorNumber += 48;
	}

	// Ruskea sijaitsee kohdassa 20?!
	else if (colorNumber == 6) {
		colorNumber = 20;
	}

	// Huom.1: V‰rit ovat rekistereiss‰ muodossa GBR.
	// Huom.2: V‰rien _LUKEMISEEN_ k‰ytet‰‰n rekisteri‰ 1015h, keskeytyst‰ 10h.
	in.w.ax = 0x1015;
	in.w.bx = colorNumber;
	int386(0x10, &in, &out);
	*r = out.h.dh;
	*g = out.h.ch;
	*b = out.h.cl;
}

/**
 * Satunnaistaa v‰rit alueella [start-stop] (molemmat mukaan luettuina).
 */
void randomizeColorRange(int start, int stop) {
	int i;
	for (i = start; i <= stop; i++) {
		setColor(i, rand() % 64, rand() % 64, rand() % 64);
	}
}

void randomizeAllColors(void) {
	randomizeColorRange(0, 15);
}

/**
 * Muuttaa v‰rin rgb-arvoja askeleen verran kohdearvojen suuntaan.
 */
void fadeToColor(int colorNumber, int r, int g, int b) {
	int oR, oG, oB;
	getColor(colorNumber, &oR, &oG, &oB);

	if (r > oR) { oR += 1; }
	else if (r < oR) { oR -= 1; }

	if (g > oG) { oG += 1; }
	else if (g < oG) { oG -= 1; }

	if (b > oB) { oB += 1; }
	else if (b < oB) { oB -= 1; }

	setColor(colorNumber, oR, oG, oB);

}

void drawScreenFromBuffer(void) {
	// katso: https://stackoverflow.com/questions/32972051/in-c-how-do-i-write-to-a-particular-memory-location-e-g-video-memory-b800-in

	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i, j;

	// Viimeiset nelj‰ heksaa ovat merkitsev‰t, alkuosa b800 on vain rekisteri, johon kirjoitetaan (eli n‰yttˆmuisti)
	// - Oikeanpuolimmaisimmat 4 bitti‰ m‰‰ritt‰v‰t etualan v‰rin (eli 0-15),
	// - seuraavat 3 bitti‰ m‰‰ritt‰v‰t taustan v‰rin (eli etualan v‰ri + n*16),
	// - viimeinen bitti m‰‰ritt‰‰ vilkkumisen (etualan ja taustan v‰ri + n*128 (eli 0 tai 128)),
	// Katso:
	// http://www.techhelpmanual.com/87-screen_attributes.html

	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			*(videomem++) = screenCharBuffer[i][j];
			*(videomem++) = screenColorBuffer[i][j];
		}
	}
}

/**
 * Piirt‰‰ n‰ytˆlle palikat suoraan blockBufferista, kulkematta screenChar- ja -colorbuffereiden kautta.
 */
void drawScreenFromBlockBuffer(void) {
	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i, j, k;

	for (i = 0; i < ROWS; i++) {
		k = i * 2;
		for (j = 0; j < COLS; j++) {
			*(videomem++) = 223;
			*(videomem++) = blockColorBuffer[k][j] + 16 * blockColorBuffer[k + 1][j];
		}
	}
}

/**
 * Piirt‰‰ blockBufferin sis‰llˆn screenChar- ja -colorBuffereihin.
 */
void drawBlocksToBuffer(void) {
	int i, j, k;

	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			k = i * 2;
			screenCharBuffer[i][j] = 223;
			screenColorBuffer[i][j] = blockColorBuffer[k][j] + 16 * blockColorBuffer[k + 1][j];
		}
	}
}

/**
 * Piirt‰‰ blockBufferin sis‰llˆn screenChar- ja -colorBuffereihin
 * paitsi jos blockBufferin "pikselin" v‰rin arvo on tpcolor.
 */
void drawTpBlocksToBuffer(char tpcolor) {
	int i, j;

	for (i = 0; i < ROWS * 2; i++) {
		for (j = 0; j < COLS; j++) {
			if (blockColorBuffer[i][j] != tpcolor){
				intelligentDrawBlockToScreenBuffer(j,i,blockColorBuffer[i][j]);
			}
		}
	}
}

/**
 * Lukee n‰yttˆmuistissa (eli ruudulla) olevan kuvan palikkaesitykseksi (merkit 219, 220 ja 223)
 * blockColorBackupBuffer()-taulukkoon.
 */
void getBlockBuffer(void) {
	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i, j, k;
	char char_temp, color_temp;

	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			char_temp = *(videomem++);
			color_temp = *(videomem++);
			k = i*2;
			
			if (char_temp == 220) {
				blockColorBackupBuffer[k][j] = color_temp / 16;
				blockColorBackupBuffer[k+1][j] = color_temp % 16;
			}
			else if (char_temp == 223) {
				blockColorBackupBuffer[k][j] = color_temp % 16;
				blockColorBackupBuffer[k+1][j] = color_temp / 16;
			}
			else if (char_temp == 32) {
				blockColorBackupBuffer[k][j] = color_temp / 16;
				blockColorBackupBuffer[k + 1][j] = color_temp / 16;
			}
			else {
				blockColorBackupBuffer[k][j] = color_temp % 16;
				blockColorBackupBuffer[k+1][j] = color_temp % 16;
			}
		}
	}
}

/**
 * Lukee n‰yttˆmuistin sis‰llˆn screen- ja colorBuffereihin.
 */
void getScreenCharColorBuffer(void) {
	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i, j;

	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			screenCharBackupBuffer[i][j] = *(videomem++);
			screenColorBackupBuffer[i][j] = *(videomem++);
		}
	}
}

/**
 * Tyhjent‰‰ screenChar- ja -colorBufferit.
 */
void clrScreenCharColorBuffer(void) {
	int i, j;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			screenCharBuffer[i][j] = 0;
			screenColorBuffer[i][j] = 0;
		}
	}
}

/**
 * T‰ytt‰‰ blockColorBufferin v‰rill‰ color.
 */
void clrBlockColorBuffer(int color) {
	int i;
	char* p = (char *)blockColorBuffer;
	for (i = 0; i < ROWS * COLS * 2; i++) {
		*(p++) = color;
	}
}

/**
 * Maalaa v‰ribufferin alueen.
 */
void paintScreenColorBufferArea(int x, int y, int w, int h, int c) {
	int i, j;
	int xlim, ylim;
	xlim = x + w;
	ylim = y + h;

	// Safeguardit ylipiirrolle:
	if (xlim >= COLS) { xlim -= COLS; w -= xlim; }
	if (ylim >= ROWS) { ylim -= ROWS; h -= ylim; }

	for (i = y; i < y + h; i++) {
		for (j = x; j < x + w; j++) {
			screenColorBuffer[i][j] = c;
		}
	}
}

/**
 * Maalaa n‰ytˆn rivinp‰tk‰n.
 */
void paintScreenRow(int x, int y, int w, int c) {
	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i, j;
	
	videomem += y * 160 + x*2 + 1;

	for (i = 0; i <= w; i++) {
		*(videomem++) = c;
		videomem++;
	}
}

/**
 * Kiert‰‰ blockbufferia d astetta (radiaaneina, naturlicht).
 */
void rotateBlockBuffer(double d) {
	int y, x, centerx, centery, m, n, j, k;
	double c, s;
	c = cos(d);
	s = sin(d);

	memcpy(transformBuffer, blockColorBuffer, sizeof(char) * 2 * ROWS * COLS);
	for (x = 0; x < COLS; x++) {
		for (y = 0; y < ROWS * 2; y++) {
			centerx = COLS / 2;
			centery = ROWS * 2 / 2;
			m = x - centerx;
			n = y - centery;
			j = ((int)(m * c + n * s)) + centerx;
			k = ((int)(n * c - m * s)) + centery;
			if (j >= 0 && j < COLS && k >= 0 && k < ROWS * 2) {
				transformBuffer[y][x] = blockColorBuffer[k][j];
			}
		}
	}
	memcpy(blockColorBuffer, transformBuffer, sizeof(char) * 2 * ROWS * COLS);
}

/**
 * Ks. scaleBlockBufferAtXY
 */
void scaleBlockBuffer(int d) {
	scaleBlockBufferAtXY(d, COLS / 2, ROWS);
}

/**
 * L‰hent‰‰ (d > 1) tai loitontaa (d < 1) blockBufferia.
 * Ei toimi t‰ysin odotetusti; ainoastaan skaalausarvo 2
 * toimii kunnolla.
 */
void scaleBlockBufferAtXY(int d, int origoX, int origoY) {
	int y, x, i, j, xc, yc, xFactor, yFactor, xs, ys, xf, yf;


	for (y = 0; y < ROWS * 2; y++) {
		for (x = 0; x < COLS; x++) {
			// Korjauskertoimia.
			// Skaalauksen keskipistett‰ pienemmille arvoille on annettava
			// koordinaattikorjaukseksi skaalauskerroin - 1 (eli esim. 2-kertaiseksi
			// skaalaamalla korjaus on 1).

			if (x < origoX) { xc = -d; xs = -1; xf = (d-1); }
			else { xc = d; xs = 1; xf = 0; }

			if (y < origoY) { yc = -d; ys = -1; yf = (d-1); }
			else { yc = d; ys = 1; yf = 0; }
			
			xFactor = (x - origoX) * xc;
			yFactor = (y - origoY) * yc;

			for (j = 0; j < d; j++) {
				for (i = 0; i < d; i++) {
					if (origoY +yf + (yFactor + j) * (ys) < 0 || origoY +yf+ (yFactor + j) * (ys) >= ROWS * 2 || origoX +xf+ (xFactor + i) * (xs) < 0 || origoX +xf+ (xFactor + i) * (xs) >= COLS) {
						;
					}
					else {
						transformBuffer[origoY +yf+ (yFactor + j) * (ys)][origoX +xf+ (xFactor + i) * (xs)] = blockColorBuffer[y][x];
					}
				}
			}
		}
	}
	memcpy(blockColorBuffer, transformBuffer, sizeof(char) * 2 * ROWS * COLS);
}

/*
 * Tulostaa bufferiin merkkijonon. Teksti wrappaa samalta rivilt‰ nollasta,
 * jos se ylitt‰‰ 79. merkin rajan.
 */
void printStringToBuffer(char* s, int x, int y) {
	char c;
	int i;
	int xy;
	xy = (x % COLS) + y * COLS;
	i = 0;
	c = s[i];

	while (c != '\0') {
		screenCharBuffer[y][(x + i < 80) ? x + i : x + i - COLS] = c;
		i++;
		c = s[i];
	}
}

/**
 * Tulostaa suoraan n‰ytˆlle merkkijonon.
 */
void printStringToScreen(char* s, char x, char y) {
	char* videomem = (char*)(SCREEN_LIN_ADDR + y*160 + x*2);
	char c;
	int i;
	int xy;
	xy = (x % COLS) + y * COLS;
	i = 0;
	c = s[i];

	while (c != '\0') {
		*(videomem++) = c;
		videomem++;
		i++;
		c = s[i];
	}
}

/**
 * Merkkijonon tulostus kera v‰rin.
 */
void printColorStringToScreen(char* s, char x, char y, char color) {
	int w = strlen(s) - 1;
	printStringToScreen(s, x, y);
	paintScreenRow(x, y, w, color);
}

/**
 * Tulostaa isofonttisen tekstin screenBufferiin. Tukee rivinvaihtoja '\n' !
 */
void printLargeStringToBuffer(int x, int y, char* s, int c) {
	int prevCharWidth, currentX;
	char a;
	
	a = *s;
	currentX = x;

	while (a != '\0') {
		if (a == '\n') {
			currentX = x;
			y += 6;
			a = *(++s);
		}
		else {
			prevCharWidth = printLargeCharToBuffer(currentX, y, a, c);
			currentX += prevCharWidth + 1;
			a = *(++s);
		}
	}
}

/**
 * Tulostaa ~3xn blokin kokoisen merkin n‰ytˆlle. Palauttaa tulostetun merkin leveyden.
 */
int printLargeCharToBuffer(int x, int y, char a, int c) {
	int i, j, k;
	int xCounter;
	int charWidth, charWidth_max;
	char cc[32];
	char ccc;

	switch (a) {
		case ' ':
			strcpy(cc, "000 000 000 000 000");
			break;
		case '!':
			strcpy(cc, "1 1 1 0 1");
			break;
		case '"':
			strcpy(cc, "101 101 000 000 000");
			break;
		case '#':
			strcpy(cc, "01010 11111 01010 11111 01010");
			break;
		case '$':
			strcpy(cc, "-010 111 100 111 001 111 010");
			break;
		case '%':
			strcpy(cc, "100 001 010 100 001");
			break;
		case '(':
			strcpy(cc, "001 010 010 010 001");
			break;
		case ')':
			strcpy(cc, "100 010 010 010 100");
			break;
		case '+':
			strcpy(cc, "000 010 111 010 000");
			break;
		case ',':
			strcpy(cc, "0 0 0 0 1 1");
			break;
		case '-':
			strcpy(cc, "000 000 111 000 000");
			break;
		case '/':
			strcpy(cc, "001 001 010 100 100");
			break;
		case '.':
			strcpy(cc, "0 0 0 0 1");
			break;
		case '0':
			strcpy(cc, "111 101 101 101 111");
			break;
		case '1':
			strcpy(cc, "010 110 010 010 010");
			break;
		case '2':
			strcpy(cc, "111 001 111 100 111");
			break;
		case '3':
			strcpy(cc, "111 001 111 001 111");
			break;
		case '4':
			strcpy(cc, "101 101 111 001 001");
			break;
		case '5':
			strcpy(cc, "111 100 111 001 111");
			break;
		case '6':
			strcpy(cc, "111 100 111 101 111");
			break;
		case '7':
			strcpy(cc, "111 001 001 001 001");
			break;
		case '8':
			strcpy(cc, "111 101 111 101 111");
			break;
		case '9':
			strcpy(cc, "111 101 111 001 001");
			break;
		case ':':
			strcpy(cc, "000 010 000 010 000");
			break;
		case ';':
			strcpy(cc, "000 010 000 010 010");
			break;
		case '<':
			strcpy(cc, "001 010 100 010 001");
			break;
		case '>':
			strcpy(cc, "100 010 001 010 100");
			break;
		case '?':
			strcpy(cc, "111 001 010 000 010");
			break;
		case '@':
			strcpy(cc, "111 111 111 100 111");
			break;
		case 'A':
			strcpy(cc, "111 101 111 101 101");
			break;
		case 'B':
			strcpy(cc, "110 101 110 101 110");
			break;
		case 'C':
			strcpy(cc, "111 100 100 100 111");
			break;
		case 'D':
			strcpy(cc, "110 101 101 101 110");
			break;
		case 'E':
			strcpy(cc, "111 100 111 100 111");
			break;
		case 'F':
			strcpy(cc, "111 100 110 100 100");
			break;
		case 'G':
			strcpy(cc, "111 100 101 101 111");
			break;
		case 'H':
			strcpy(cc, "101 101 111 101 101");
			break;
		case 'I':
			strcpy(cc, "111 010 010 010 111");
			break;
		case 'J':
			strcpy(cc, "001 001 001 101 111");
			break;
		case 'K':
			strcpy(cc, "101 101 110 101 101");
			break;
		case 'L':
			strcpy(cc, "100 100 100 100 111");
			break;
		case 'M':
			strcpy(cc, "11111 10101 10101 10001 10001");
			break;
		case 'N':
			strcpy(cc, "110 101 101 101 101");
			break;
		case 'O':
			strcpy(cc, "111 101 101 101 111");
			break;
		case 'P':
			strcpy(cc, "111 101 111 100 100");
			break;
		case 'Q':
			strcpy(cc, "111 101 101 101 111 001");
			break;
		case 'R':
			strcpy(cc, "110 101 110 101 101");
			break;
		case 'S':
			strcpy(cc, "111 100 111 001 111");
			break;
		case 'T':
			strcpy(cc, "111 010 010 010 010");
			break;
		case 'U':
			strcpy(cc, "101 101 101 101 111");
			break;
		case 'V':
			strcpy(cc, "101 101 101 101 010");
			break;
		case 'W':
			strcpy(cc, "10001 10001 10101 10101 11111");
			break;
		case 'X':
			strcpy(cc, "101 101 010 101 101");
			break;
		case 'Y':
			strcpy(cc, "101 101 010 010 010");
			break;
		case 'Z':
			strcpy(cc, "111 001 010 100 111");
			break;	
		
		default:
			strcpy(cc, "111 111 111 111 111");
			break;

	}
	
	i = x; j = y; k = 0; xCounter = 0; charWidth = 0; charWidth_max = 0;
	ccc = cc[k];
	while (ccc != '\0') {
		if (ccc == '1') {
			blockColorBuffer[j][i] = c;
			i++;
			xCounter++;
			charWidth++;
		}
		else if (ccc == '0') {
			i++;
			xCounter++;
			charWidth++;
		}
		else if (ccc == '-') {
			j--;
		}
		else {
			j++;
			i -= xCounter;
			xCounter = 0;

			if (charWidth > charWidth_max) {
				charWidth_max = charWidth;
			}
			charWidth = 0;
		}
		k++;
		ccc = cc[k];
	}
	
	return charWidth_max;
}

/**
 * Piirt‰‰ palikkamerkkej‰ (220 tai 223) screenChar- ja -ColorBuffereihin
 * huomioiden bufferissa jo olevan sis‰llˆn ja s‰‰t‰en taustav‰rin sen mukaan.
 */
void intelligentDrawBlockToScreenBuffer(int x, int y, int c) {

	// Safeguard.
	if (y >= ROWS * 2 || x >= COLS || y < 0 || x < 0) {
		;
	}

	else{

		int realY = y / 2;

		if (y % 2 == 1) {
			if (screenCharBuffer[realY][x] == 223 || screenCharBuffer[realY][x] == 219) {
				screenCharBuffer[realY][x] = 223;
				screenColorBuffer[realY][x] = (screenColorBuffer[realY][x] % 16) + c * 16;
			}
			else {
				screenCharBuffer[realY][x] = 220;
				screenColorBuffer[realY][x] = c + 16 * (screenColorBuffer[realY][x] / 16);
			}
		}
		else {
			if (screenCharBuffer[realY][x] == 220 || screenCharBuffer[realY][x] == 219) {
				screenCharBuffer[realY][x] = 220;
				screenColorBuffer[realY][x] = (screenColorBuffer[realY][x] % 16) + c * 16;
			}
			else {
				screenCharBuffer[realY][x] = 223;
				screenColorBuffer[realY][x] = c + 16 * (screenColorBuffer[realY][x] / 16);
			}
		}
	}
}

/**
 * Siirt‰‰ blockBufferin sis‰ltˆ‰ x ja y askelta haluttuun suuntaan. Wrappaa.
 * Hyv‰ksik‰ytt‰‰ transformBufferia.
 */
void shiftBlockBuffer(int x, int y) {
	int xSize, ySize, i, j;
	if (x != 0) {
		if (x < 0) {
			xSize = -x;
			// Ensin tallennetaan leikattava osio.
			for (j = 0; j < ROWS * 2; j++) {
				for (i = 0; i < xSize; i++) {
					transformBuffer[j][i] = blockColorBuffer[j][i];
				}
			}
			// Shiftataan blockColorBufferia.
			for (j = 0; j < ROWS * 2; j++) {
				for (i = 0; i < COLS; i++) {
					if (i + xSize < COLS) {
						blockColorBuffer[j][i] = blockColorBuffer[j][i + xSize];
					}
					else {
						blockColorBuffer[j][i] = transformBuffer[j][(i+xSize)%COLS];
					}
				}
			}

		}
		else {
			xSize = x;
			// Ensin tallennetaan leikattava osio.
			for (j = 0; j < ROWS * 2; j++) {
				for (i = COLS-xSize; i < COLS; i++) {
					transformBuffer[j][i] = blockColorBuffer[j][i];
				}
			}
			// Shiftataan blockColorBufferia.
			for (j = 0; j < ROWS * 2; j++) {
				for (i = COLS-1; i > -1; i--) {
					if (i - xSize > -1) {
						blockColorBuffer[j][i] = blockColorBuffer[j][i-xSize];
					}
					else {
						blockColorBuffer[j][i] = transformBuffer[j][COLS+((i-xSize)%COLS)];
					}
				}
			}

		}

	}
	if (y != 0) {
		if (y < 0) {
			ySize = -y;
			// Ensin tallennetaan leikattava osio.
			for (j = 0; j < ySize; j++) {
				for (i = 0; i < COLS; i++) {
					transformBuffer[j][i] = blockColorBuffer[j][i];
				}
			}
			// Shiftataan blockColorBufferia.
			for (j = 0; j < ROWS * 2; j++) {
				for (i = 0; i < COLS; i++) {
					if (j + ySize < ROWS*2) {
						blockColorBuffer[j][i] = blockColorBuffer[j + ySize][i];
					}
					else {
						blockColorBuffer[j][i] = transformBuffer[(j + ySize) % (ROWS*2)][i];
					}
				}
			}

		}
		else {
			ySize = y;
			// Ensin tallennetaan leikattava osio.
			for (j = ROWS * 2 - ySize; j < ROWS * 2; j++) {
				for (i = 0; i < COLS; i++) {
					transformBuffer[j][i] = blockColorBuffer[j][i];
				}
			}
			// Shiftataan blockColorBufferia.
			for (j = ROWS*2 - 1; j > -1; j--) {
				for (i = 0; i < COLS; i++) {
					if (j - ySize > -1) {
						blockColorBuffer[j][i] = blockColorBuffer[j - ySize][i];
					}
					else {
						blockColorBuffer[j][i] = transformBuffer[(ROWS*2) + ((j - ySize) % (ROWS*2))][i];
					}
				}
			}

		}

	}
}

/**
 * Shiftaa blockbufferin rivi‰ miinusmerkkisesti vasemmalle tai plusmerkkisesti oikealle i:n verran.
 */
void shiftBlockBufferRow(int row, int amount) {
	int j;
	if (amount > 0) {
		for (j = 0; j < amount; j++) {
			shiftBlockBufferRowRight(row);
		}
	}
	else if (amount < 0) {
		amount = -amount;
		for (j = 0; j < amount; j++) {
			shiftBlockBufferRowLeft(row);
		}
	}
	else {
		;
	}
}

/**
 * Shiftaa blockbufferia merkin verran vasemmalle.
 */
void shiftBlockBufferRowLeft(int row) {
	int i;
	char a;
	a = blockColorBuffer[row][0];
	for (i = 0; i < COLS - 1; i++) {
		blockColorBuffer[row][i] = blockColorBuffer[row][i + 1];
	}
	blockColorBuffer[row][COLS - 1] = a;
}

/**
 * Shiftaa blockbufferia merkin verran oikealle.
 */

void shiftBlockBufferRowRight(int row) {
	int i;
	char a;
	a = blockColorBuffer[row][COLS - 1];
	for (i = COLS - 1; i > 0; i--) {
		blockColorBuffer[row][i] = blockColorBuffer[row][i - 1];
	}
	blockColorBuffer[row][0] = a;
}

void shiftBlockBufferCol(int col, int amount) {
	int j;
	if (amount > 0) {
		for (j = 0; j < amount; j++) {
			shiftBlockBufferColDown(col);
		}
	}
	else if (amount < 0) {
		amount = -amount;
		for (j = 0; j < amount; j++) {
			shiftBlockBufferColUp(col);
		}
	}
	else {
		;
	}
}

void shiftBlockBufferColUp(int col) {
	int i;
	char a;
	a = blockColorBuffer[0][col];
	for (i = 0; i < ROWS*2 - 1; i++) {
		blockColorBuffer[i][col] = blockColorBuffer[i + 1][col];
	}
	blockColorBuffer[ROWS*2 - 1][col] = a;
}

void shiftBlockBufferColDown(int col) {
	int i;
	char a;
	a = blockColorBuffer[ROWS*2 - 1][col];
	for (i = ROWS*2 - 1; i > 0; i--) {
		blockColorBuffer[i][col] = blockColorBuffer[i - 1][col];
	}
	blockColorBuffer[0][col] = a;
}

/**
 * Piirt‰‰ t‰ytetyn suorakaiteen screenChar- ja screenColorbuffereihin.
 */

void fillRect(int x, int y, int w, int h, int color) {
	int i, j, xlim, ylim, realJ;

	xlim = x + w;
	ylim = y + h;

	// Safeguardit ylipiirrolle:
	if (xlim >= COLS) { xlim -= COLS; w -= xlim; }
	if (ylim >= ROWS*2) { ylim -= ROWS*2; h -= ylim; }

	for (j = y; j < y + h; j++) {
		// 223 on yl‰palikka, 220 on alapalikka
		// Tarkistetaan, onko piirtokohteessa jo palikka.
		for (i = x; i < x + w; i++) {
			realJ = j/2;
			if (j == y && j % 2 == 1){
				if (screenCharBuffer[realJ][i] == 223 || screenCharBuffer[realJ][i] == 219) {
					screenCharBuffer[realJ][i] = 223;
					screenColorBuffer[realJ][i] = (screenColorBuffer[realJ][i] % 16) + color * 16;
				}
				else {
					screenCharBuffer[realJ][i] = 220;
					screenColorBuffer[realJ][i] = color + 16 * (screenColorBuffer[realJ][i] / 16);
				}
			}
			else if (j == y + h-1 && j % 2 == 0) {
				if (screenCharBuffer[realJ][i] == 220 || screenCharBuffer[realJ][i] == 219) {
					screenCharBuffer[realJ][i] = 220;
					screenColorBuffer[realJ][i] = (screenColorBuffer[realJ][i] % 16) + color * 16;
				}
				else {
					screenCharBuffer[realJ][i] = 223;
					screenColorBuffer[realJ][i] = color + 16 * (screenColorBuffer[realJ][i] / 16);
				}
			}
			else {
				screenCharBuffer[realJ][i] = 219;
				screenColorBuffer[realJ][i] = color;
			}
		}

	}

}

/**
 * Piirt‰‰ t‰ytetyn suorakaiteen blockBufferiin. Ei sis‰ll‰ "turhia" ylimenotarkistuksia.
 */
void fillRectToBlockBuffer(int x, int y, int w, int h, int color) {
	int j, i;
	for (j = y; j < y + h; j++) {
		for (i = x; i < x + w; i++) {
			blockColorBuffer[j][i] = color;
		}
	}
}

/**
 * Piirt‰‰ tyhj‰n suorakaiteen blockBufferiin. Ei ylimenotarkistuksia.
 */
void strokeRectToBlockBuffer(int x, int y, int w, int h, int color) {
	int j;
	for (j = y; j <= y + h; j++) {
		blockColorBuffer[j][x] = color;
		blockColorBuffer[j][x + w] = color;
	}

	for (j = x; j <= x + w; j++) {
		blockColorBuffer[y][j] = color;
		blockColorBuffer[y + h][j] = color;
	}
}

/**
 * Piirt‰‰ ympyr‰n blockBufferiin.
 */
void strokeCircleToBlockBuffer(int x, int y, int radius, int color) {
	int i, j, k, cy = y - radius, cx = x - radius;
	static int cy_a[4];
	static int cx_a[4];
	static int fx_a[4];
	static int fy_a[4];
	double d;

	cy_a[0] = y - radius;
	cy_a[1] = y + radius;
	cy_a[2] = y - radius;
	cy_a[3] = y + radius;
	cx_a[0] = x + radius;
	cx_a[1] = x + radius;
	cx_a[2] = x - radius;
	cx_a[3] = x - radius;
	fy_a[0] = 1;
	fy_a[1] = -1;
	fy_a[2] = 1;
	fy_a[3] = -1;
	fx_a[0] = -1;
	fx_a[1] = -1;
	fx_a[2] = 1;
	fx_a[3] = 1;

	// Lukee vain yhden nelj‰nneksen koordinaatit ja nelist‰‰ ne.
	for (i = 0; i <= radius; i++) {
		for (j = 0; j <= radius; j++) {
			d = sqrt((double)(i - radius) * (i - radius) + (j - radius) * (j - radius));
			if (d > radius - 0.5 && d < radius + 0.5) {
				for (k = 0; k < 4; k++) {
					if (cy_a[k] + i*fy_a[k] < 0 || cx_a[k] + j*fx_a[k] < 0 || cy_a[k] + i*fy_a[k] >= ROWS * 2 || cx_a[k] + j * fx_a[k] >= COLS) {
						;
					}
					else {
						blockColorBuffer[cy_a[k] + i*fy_a[k]][cx_a[k] + j*fx_a[k]] = color;
					}
				}
				
			}
		}
	}
}

void fillCircleToBlockBuffer(int x, int y, int radius, int color) {
	int i, j, k, fillOn, cy = y - radius, cx = x - radius;
	static int cy_a[4];
	static int cx_a[4];
	static int fx_a[4];
	static int fy_a[4];
	double d;

	cy_a[0] = y - radius;
	cy_a[1] = y + radius;
	cy_a[2] = y - radius;
	cy_a[3] = y + radius;
	cx_a[0] = x + radius;
	cx_a[1] = x + radius;
	cx_a[2] = x - radius;
	cx_a[3] = x - radius;
	fy_a[0] = 1;
	fy_a[1] = -1;
	fy_a[2] = 1;
	fy_a[3] = -1;
	fx_a[0] = -1;
	fx_a[1] = -1;
	fx_a[2] = 1;
	fx_a[3] = 1;

	// Lukee vain yhden nelj‰nneksen koordinaatit ja nelist‰‰ ne.
	for (i = 0; i <= radius; i++) {
		fillOn = 0;
		for (j = 0; j <= radius; j++) {
			if (fillOn == 0) {
				d = sqrt((double)(i - radius) * (i - radius) + (j - radius) * (j - radius));
				if (d > radius - 0.5 && d < radius + 0.5) {
					fillOn = 1;
				}
			}
			if (fillOn == 1) {
					for (k = 0; k < 4; k++) {
						if (cy_a[k] + i * fy_a[k] < 0 || cx_a[k] + j * fx_a[k] < 0 || cy_a[k] + i * fy_a[k] >= ROWS * 2 || cx_a[k] + j * fx_a[k] >= COLS) {
							;
						}
						else {
							blockColorBuffer[cy_a[k] + i * fy_a[k]][cx_a[k] + j * fx_a[k]] = color;
						}
					}
			}
		}
	}
}

/**
 * Piirt‰‰ kolmion blockBufferiin.
 */
void triangleToBlockBuffer(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
	lineToBlockBuffer(x0, y0, x1, y1, color);
	lineToBlockBuffer(x1, y1, x2, y2, color);
	lineToBlockBuffer(x2, y2, x0, y0, color);
}

/**
 * Piirt‰‰ viivan blockBufferiin. Hyv‰ksik‰ytt‰‰ puhtaasti Bresenhamin algoritmia.
 */
void lineToBlockBuffer(int x0, int y0, int x1, int y1, int color) {
	int i;
	// Lis‰t‰‰n kohtisuorille viivoille t‰mmˆinen nopeutus:
	if (y0 == y1) {
		if (x0 == x1) {
			blockColorBuffer[y0][x0] = color;
		}
		else {
			if (x0 > x1) {
				for (i = x0; i >= x1; i--) {
					blockColorBuffer[y0][i] = color;
				}
			}
			else {
				for (i = x0; i <= x1; i++) {
					blockColorBuffer[y0][i] = color;
				}
			}
		}
	}

	else if (x0 == x1) {
		if (y0 == y1) {
			blockColorBuffer[y0][x0] = color;
		}
		else {
			if (y0 > y1) {
				for (i = y0; i >= y1; i--) {
					blockColorBuffer[i][x0] = color;
				}
			}
			else {
				for (i = y0; i <= y1; i++) {
					blockColorBuffer[i][x0] = color;
				}
			}
		}
	}

	else {
		if (abs(y1 - y0) < abs(x1 - x0)) {
			if (x0 > x1) {
				lineToBlockBufferLow(x1, y1, x0, y0, color);
			}
			else {
				lineToBlockBufferLow(x0, y0, x1, y1, color);
			}
		}
		else {
			if (y0 > y1) {
				lineToBlockBufferHigh(x1, y1, x0, y0, color);
			}
			else {
				lineToBlockBufferHigh(x0, y0, x1, y1, color);
			}
		}
	}
}

void lineToBlockBufferLow(int x0, int y0, int x1, int y1, int color) {
	int dx, dy, yi, y, x, D;
	dx = x1 - x0;
	dy = y1 - y0;
	yi = 1;
	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}
	D = 2 * dy - dx;
	y = y0;

	for (x = x0; x <= x1; x++) {
		blockColorBuffer[y][x] = color;
		if (D > 0) {
			y += yi;
			D -= 2 * dx;
		}
		D += 2 * dy;
	}
}

void lineToBlockBufferHigh(int x0, int y0, int x1, int y1, int color) {
	int dx, dy, xi, x, y, D;
	dx = x1 - x0;
	dy = y1 - y0;
	xi = 1;
	if (dx < 0) {
		xi = -1;
		dx = -dx;
	}
	D = 2 * dx - dy;
	x = x0;

	for (y = y0; y <= y1; y++) {
		blockColorBuffer[y][x] = color;
		if (D > 0) {
			x += xi;
			D -= 2 * dy;
		}
		D += 2 * dx;
	}

}

/**
 * Ruudun tyhj‰ys (tai siis 0-v‰rill‰ t‰yttˆ). Jos haluat nollata sek‰ paletin
 * ett‰ tyhjent‰‰ ruudun, k‰yt‰ initTextMode()-funktiota.
 */
void clrScr(void){
	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i;
	for (i = 0; i < ROWS * COLS * 2; i++) {
		*(videomem++) = 0;
	}
}

/**
 * M‰‰ritt‰‰, onko textmodessa k‰ytˆss‰ vilkkuvat v‰rit (true) (default) vai
 * t‰ydet 16 taustav‰ri‰ (false).
 */
void setBlinking(bool b) {

	int a;

	union REGS in, out;

	if (b) { a = 0x01; }
	else { a = 0x00; }

	in.w.ax = 0x1003;
	in.h.bl = a;
	int386(0x10, &in, &out);
}

/**
 * N‰ytt‰‰ tai piilottaa tekstimoodin vilkkuvan osoittimen.
 * Katso: https://wiki.osdev.org/Text_Mode_Cursor
 */
void showCursor(bool b) {
	union REGS in, out;
	
	in.h.ah = 0x01;

	if (b) {
		in.h.ch = 14;
		in.h.cl = 15;
		int386(0x10, &in, &out);
	}

	else {
		in.h.ch = 0x3f;
		int386(0x10, &in, &out);
	}
}

/**
 * Fonttien muokkaus
 * http://www.techhelpmanual.com/152-int_10h_1100h__load_user_defined_font.html
 */
void defineChar(int cnum, char* fontData) {
	union REGS regs;
	struct SREGS sregs;
	int interrupt_no = 0x10;
	char *str;

	/**
	 * T‰ytt‰ taetta siit‰, ett‰ t‰m‰ ei kirjoittaisi v‰‰riin rekistereihin jotain,
	 * ei ole, sill‰ en ymm‰rr‰ toteutusta t‰ysin. K‰yt‰ varoen.
	 */

	memset(&sregs, 0, sizeof(sregs));
	str = (char*)(MK_FP(regs.w.dx, 0));
	_fstrcpy(str, fontData);
	regs.w.ax = 0x1100;
	regs.h.bl = 0;
	regs.h.bh = 16;
	regs.w.cx = 1;
	regs.w.dx = cnum;
	int386x(interrupt_no, &regs, &regs, &sregs);
	

}

/**
 * Lataa ANSI-grafiikkaa sis‰lt‰v‰n .BIN-tiedoston n‰ytˆlle.
 * HUOM.! Luettavaa kuvadataa saa olla enint‰‰n 25 rivin verran!
 */

void loadAnsiToImageBuffer(char* filename) {
	/**
	 * Sattumalta .BIN-muotoisten ANSI-grafiikkatiedostojen muoto on 1:1 sama
	 * kuin yhdistettyjen screenChar- ja -colorBuffereiden, eli joka nollas
	 * merkki tiedostossa on tulostettava merkki ja joka yhdes merkki on
	 * edelt‰v‰n tulostettavan merkin v‰ri- ja muut m‰‰reet (eli k‰yt‰nnˆss‰
	 * siis v‰ri). 
	 */
	int i;
	char c;
	char* buf_p = imageBuffer;
	char* buffer = 0;
	//char test[10];
	long length;
	FILE* f = fopen(filename, "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = (char*)(malloc(length));
		if (buffer) {
			fread(buffer, 1, length, f);
		}
		fclose(f);
	}

	if (buffer) {
		i = 0;
		c = buffer[i];
		while (c != 26) {
			*(buf_p++) = c;
			c = buffer[++i];
		}
	}
	free(buffer);
}

void drawScreenFromImageBuffer(bool transparency) {
	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i;
	char c;

	// Jos merkki on 32 (eli v‰lilyˆnti), ja transparency on true,
	// merkki‰ ei piirret‰.
	if (transparency) {
		for (i = 0; i < ROWS * COLS * 2; i++) {
			if (i % 2 == 0 && imageBuffer[i] == 32) {
				videomem += 2;
				i++;
			}
			else {
				*(videomem++) = imageBuffer[i];
			}
		}
	}

	else {
		for (i = 0; i < ROWS * COLS * 2; i++) {
			*(videomem++) = imageBuffer[i];
		}
	}
}


/**
 * Tallentaa n‰yttˆmuistin sis‰llˆn imageBufferiin.
 */
void saveScreenToImageBuffer() {
	char* videomem = (char*)SCREEN_LIN_ADDR;
	int i;

	for (i = 0; i < ROWS * COLS * 2; i++) {
		imageBuffer[i] = *(videomem++);
	}
}

/**
 * Kopioi imageBufferin sis‰llˆn screenChar- ja -colorBuffereihin.
 */
void copyImageBufferToScreenBuffer(bool transparency) {
	int i, j, k;
	char c;

	// Jos merkki on 32 (eli v‰lilyˆnti), ja transparency on true,
	// merkki‰ ei piirret‰.
	if (transparency) {
		for (i = 0; i < ROWS * COLS * 2; i+=2) {
			if (i % 2 == 0 && imageBuffer[i] == 32) {
				i+=2;
			}
			else {
				j = i / (COLS * 2);
				k = (i % 160) / 2;
				screenCharBuffer[j][k] = imageBuffer[i];
				screenColorBuffer[j][k] = imageBuffer[i + 1];
			}
		}
	}

	else {
		for (i = 0; i < ROWS * COLS * 2; i+=2) {
			j = i / (COLS * 2);
			k = (i % 160) / 2;
			screenCharBuffer[j][k] = imageBuffer[i];
			screenColorBuffer[j][k] = imageBuffer[i + 1];
		}
	}
}