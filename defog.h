#ifndef _DEFOG_H_
#define _DEFOG_H_

#include <iostream>
#include <windows.h> 

#define u8max (255)
#define u8min (0)
#define calc_min(a,b) ((a)>(b)?(b):(a))
#define calc_max(a,b) ((a)<(b)?(b):(a))

typedef struct _RGB
{
	BYTE b;
	BYTE g;
	BYTE r;
}RGB;

typedef struct
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
}RGBA;


int img_darken(RGB* img);

int img_process(RGB* img);

RGB* load_bmp(const char* filename);

void save_bmp(const char* filename, RGB* img);


#endif