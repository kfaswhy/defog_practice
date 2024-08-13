#ifndef _DEFOG_H_
#define _DEFOG_H_

#include <iostream>
#include <windows.h> 

#define U32 unsigned int
#define U8 unsigned char 
#define U8MAX (255)
#define U8MIN (0)
#define calc_min(a,b) ((a)>(b)?(b):(a))
#define calc_max(a,b) ((a)<(b)?(b):(a))

#define LOG(...) printf("%s [%d]: ", __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");

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