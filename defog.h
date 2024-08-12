#ifndef _DEFOG_H_
#define _DEFOG_H_

#include <iostream>
#include <windows.h> 
#include "bmp_process.h"
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


RGB* load_bmp(const char* filename);

void save_bmp(const char* filename, RGB* img);

int img_process(RGB* img);

#endif