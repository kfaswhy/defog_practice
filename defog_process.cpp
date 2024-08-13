#include "defog.h"

extern BITMAPFILEHEADER fileHeader;
extern BITMAPINFOHEADER infoHeader;

extern int height = 0;
extern int width = 0;
extern int PaddingSize = 0;
extern BYTE* pad = NULL;

int img_darken(RGB* img)
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int index = i * width + j;
			/* ÏñËØÔËËã */
			img[index].r = img[index].r >> 1;
			img[index].g = img[index].g >> 0;
			img[index].b = img[index].b >> 1;


			/* ÏñËØÔËËã½áÊø */
		}
	}
	return 0;
}

int img_process(RGB* img)
{
	img_darken(img);
	return 0;
}