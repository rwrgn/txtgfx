#ifndef TXTGFX_H
#define TXTGFX_H

#include <string.h>
#include <conio.h>

// uint32_t yms. tietotyyppit:
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <i86.h>
#include <stdbool.h>

#include <math.h>
#include <time.h>

// Näytön koko ja näytön päivityksen millisekuntitahdistus
#define ROWS 25
#define COLS 80
#define REFRESH_MS 16

// Näyttömuistin osoite 32-bittistä kääntäjää varten
#define SCREEN_AREA 0xb800
#define SCREEN_LIN_ADDR ((SCREEN_AREA) << 4)

void setColor(int colorNumber, int r, int g, int b);
void getColor(int colorNumber, int* r, int* g, int* b);
void randomizeColorRange(int start, int stop);
void randomizeAllColors(void);

void fadeToColor(int colorNumber, int r, int g, int b);

void setBlinking(bool b);

void showCursor(bool b);

// Assembler-funktio kokonaisuudessaan tässä tiedostossa.
void initTextMode(void);

void clrScr(void);

void drawScreenFromBuffer(void);
void drawScreenFromBlockBuffer(void);

void drawBlocksToBuffer(void);
void printStringToBuffer(char* s, int x, int y);
void printStringToScreen(char* s, char x, char y);
void printColorStringToScreen(char* s, char x, char y, char color);

void getBlockBuffer(void);
void getScreenCharColorBuffer(void);

void scaleBlockBuffer(int d);
void scaleBlockBufferAtXY(int d, int origoX, int origoY);
void shiftBlockBuffer(int x, int y);
void rotateBlockBuffer(double d);

void shiftBlockBufferRow(int row, int amount);
void shiftBlockBufferRowLeft(int row);
void shiftBlockBufferRowRight(int row);

void shiftBlockBufferCol(int col, int amount);
void shiftBlockBufferColUp(int col);
void shiftBlockBufferColDown(int col);

void clrScreenCharColorBuffer(void);
void clrBlockColorBuffer(int color);

void paintScreenColorBufferArea(int x, int y, int w, int h, int c);
void paintScreenRow(int x, int y, int w, int c);

// Grafiikkaprimitiivine piirto yms.
void fillRect(int x, int y, int w, int h, int color);
void fillRectToBlockBuffer(int x, int y, int w, int h, int color);
void strokeRectToBlockBuffer(int x, int y, int w, int h, int color);

void fillCircleToBlockBuffer(int x, int y, int radius, int color);
void strokeCircleToBlockBuffer(int x, int y, int radius, int color);

void triangleToBlockBuffer(int x0, int y0, int x1, int y1, int x2, int y2, int color);

void lineToBlockBuffer(int x0, int y0, int x1, int y1, int color);
void lineToBlockBufferLow(int x0, int y0, int x1, int y1, int color);
void lineToBlockBufferHigh(int x0, int y0, int x1, int y1, int color);

void intelligentDrawBlockToScreenBuffer(int x, int y, int c);

int printLargeCharToBuffer(int x, int y, char a, int c);
void printLargeStringToBuffer(int x, int y, char* s, int c);

// Fonttien muokkaus:
void defineChar(int cnum, char* fontData);

// Kuvatiedostojen lataus:
void loadAnsiToImageBuffer(char* filename);
void drawScreenFromImageBuffer(bool transparency);
void saveScreenToImageBuffer(void);

typedef struct rgbColor {
	int r;
	int g;
	int b;
};

// Näyttöbufferit:
extern char screenCharBuffer[ROWS][COLS];
extern char screenCharBackupBuffer[ROWS][COLS];
extern char screenColorBuffer[ROWS][COLS];
extern char screenColorBackupBuffer[ROWS][COLS];
extern char blockColorBuffer[2 * ROWS][COLS];
extern char blockColorBackupBuffer[2 * ROWS][COLS];
extern char transformBuffer[2 * ROWS][COLS];

// Bufferit bin-kuville.
extern char imageBuffer[2 * ROWS * COLS];

/**
 * Tekstimoodin alustus ja ruudun tyhjäys assemblerilla. Nollaa myös paletin.
 * Katso: http://www.techhelpmanual.com/114-video_modes.html
 *
 * Huomionarvoista: koko funktio on .h-tiedostossa, koska Watcomin kääntäjällä
 * sen käyttöön saaminen ulkoisissa .c-tiedostoissa muulla tavoin on
 * ongelmallista.
 */
#pragma aux initTextMode =			\
	"mov ax, 0x03",					\
	"int 0x10"

#endif
