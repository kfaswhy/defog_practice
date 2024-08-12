#ifndef _BMP_PROCESS_H_
#define _BMP_PROCESS_H_
#include <iostream>
#include <windows.h> 
#include "defog.h"

typedef struct
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

int img_process(RGB* img);

void save_bmp(const char* filename, RGB* img);

#endif