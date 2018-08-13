/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include "hankaku.h"
#include "image.h"

extern int bpp;
extern int factor;

void printScreen(Image *source, int x, int y, int width, int height, int alpha, s_screen *destination)
{
    unsigned *dp, *sp;
    unsigned(*blendfp)(unsigned, unsigned);
    int dy;

    #define __putpixel32(p) \
                if(blendfp )\
                {\
                    *(p) = blendfp(colour, *(p));\
                }\
                else\
                {\
                    *(p) = colour;\
                }

    dp = ((unsigned *)destination->data) + (y * destination->width + x);
    sp = (void *)source->data;

    blendfp = getblendfunction32(alpha);

    for(dy = 0; dy < height; dy++)
    {
        for(x = 0; x < width; x++)
        {
            unsigned int colour, s_color;

            s_color = sp[x] & 0x00FFFFFF;

            colour = (unsigned int)s_color;
            __putpixel32(dp);
            dp++;
        }
        dp += (destination->width - width);
        sp += source->imageWidth;
    }

    #undef __putpixel32
}

void printBox(int x, int y, int width, int height, unsigned int colour, int alpha, s_screen *destination)
{
    unsigned *cp;
    unsigned(*blendfp)(unsigned, unsigned);

    #define __putpixel32(p) \
                if(blendfp )\
                {\
                    *(p) = blendfp(colour, *(p));\
                }\
                else\
                {\
                    *(p) = colour;\
                }

    if(width <= 0)
    {
        return;
    }
    if(height <= 0)
    {
        return;
    }
    if(destination == NULL)
    {
        return;
    }

    if(x < 0)
    {
        if((width += x) <= 0)
        {
            return;
        }
        x = 0;
    }
    else if(x >= destination->width)
    {
        return;
    }
    if(y < 0)
    {
        if((height += y) <= 0)
        {
            return;
        }
        y = 0;
    }
    else if(y >= destination->height)
    {
        return;
    }
    if(x + width > destination->width)
    {
        width = destination->width - x;
    }
    if(y + height > destination->height)
    {
        height = destination->height - y;
    }

    cp = ((unsigned *)destination->data) + (y * destination->width + x);
    colour &= 0x00FFFFFF;

    blendfp = getblendfunction32(alpha);

    while(--height >= 0)
    {
        for(x = 0; x < width; x++)
        {
            __putpixel32(cp);
            cp++;
        }
        cp += (destination->width - width);
    }

    #undef __putpixel32
}

void printText(s_screen *destination, int x, int y, int col, int backcol, int fill, char *format, ...)
{
	int x1, y1, i;
	unsigned long data;
	unsigned short *line16 = NULL;
	unsigned long  *line32 = NULL;
	unsigned char *font;
	unsigned char ch = 0;
	char buf[128] = {""};
	va_list arglist;
		va_start(arglist, format);
		vsprintf(buf, format, arglist);
		va_end(arglist);
	if(factor > 1){ y += 5; }

	for(i=0; i<sizeof(buf); i++)
	{
		ch = buf[i];
		// mapping
		if (ch<0x20) ch = 0;
		else if (ch<0x80) { ch -= 0x20; }
		else if (ch<0xa0) {	ch = 0;	}
		else ch -= 0x40;
		font = (u8 *)&hankaku_font10[ch*10];
		// draw
		if (bpp == 16) line16 = (unsigned short *)destination->data + x + y * destination->width;
		else           line32 = (unsigned long  *)destination->data + x + y * destination->width;

		for (y1=0; y1<10; y1++)
		{
			data = *font++;
			for (x1=0; x1<5; x1++)
			{
				if (data & 1)
				{
					if (bpp == 16) *line16 = col;
				    else           *line32 = col;
				}
				else if (fill)
				{
					if (bpp == 16) *line16 = backcol;
					else           *line32 = backcol;
				}

				if (bpp == 16) line16++;
				else           line32++;

				data = data >> 1;
			}
			if (bpp == 16) line16 += destination->width-5;
			else           line32 += destination->width-5;
		}
		x+=5;
	}
}

static int getNextPower2(int width)
{
	int b = width;
	int n;
	for(n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if(b == 2 * width) b >>= 1;
	return b;
}

Image* createImage(int width, int height)
{
	Image* image = (Image*) malloc(sizeof(Image));
	if(!image) return NULL;
	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);
	image->data = (Color*) malloc(image->imageWidth * image->imageHeight * sizeof(Color));
	if(!image->data)
	{
		free(image);
		image = NULL;
		return NULL;
	}
	memset(image->data, 0, image->imageWidth * image->imageHeight * sizeof(Color));
	return image;
}

void freeImage(Image* image)
{
	free(image->data);
	image->data = NULL;
	free(image);
	image = NULL;
}

void clearImage(Image* image, Color color)
{
	int i;
	int size = image->imageWidth * image->imageHeight;
	Color* data = image->data;
	for(i=0; i<size; i++, data++) *data = color;
}

void putPixelToImage(Image* image, Color color, int x, int y)
{
	image->data[x + y * image->imageWidth] = color;
}

Color getPixelFromImage(Image* image, int x, int y)
{
	return image->data[x + y * image->imageWidth];
}

void copyImageToImage(int sx, int sy, int width, int height, Image* source, int dx, int dy, Image* destination)
{
	Color* destinationData = &destination->data[destination->imageWidth * dy + dx];
	int destinationSkipX = destination->imageWidth - width;
	Color* sourceData = &source->data[source->imageWidth * sy + sx];
	int sourceSkipX = source->imageWidth - width;
	int x, y;
	for(y=0; y<height; y++, destinationData+=destinationSkipX, sourceData+=sourceSkipX)
	{
		for(x=0; x<width; x++, destinationData++, sourceData++) *destinationData = *sourceData;
	}
}
