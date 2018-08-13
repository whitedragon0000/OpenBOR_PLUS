/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "types.h"

typedef unsigned Color;
typedef struct
{
	int textureWidth;
	int textureHeight;
	int imageWidth;
	int imageHeight;
	Color* data;
} Image; // __attribute__ ((aligned (16)))

void printScreen(Image *source, int x, int y, int width, int height, int alpha, s_screen *destination);
void printBox(int x, int y, int width, int height, unsigned int colour, int alpha, s_screen *destination);
void printText(s_screen *destination, int x, int y, int col, int backcol, int fill, char *format, ...);
Image* createImage(int width, int height);
void freeImage(Image* image);
void clearImage(Image* image, Color color);
void putPixelToImage(Image* image, Color color, int x, int y);
Color getPixelFromImage(Image* image, int x, int y);
void copyImageToImage(int sx, int sy, int width, int height, Image* source, int dx, int dy, Image* destination);

#endif
