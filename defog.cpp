#include <iostream>
#include "defog.h"
#include "bmp_process.h"

BITMAPFILEHEADER fileHeader;
BITMAPINFOHEADER infoHeader;

int ImageHeight = 0;
int ImageWidth = 0;
int PaddingSize = 0;
BYTE* pad = NULL;

int main()
{
	char bmp_in[] = "C:/Work/Desktop/1.bmp";
	char bmp_out[] = "C:/Work/Desktop/2.bmp";
	RGB* img = NULL;
	img = load_bmp(bmp_in);
	img_process(img);
	save_bmp(bmp_out, img);
	return 0;
}

int img_process(RGB* img)
{
	for (int i = 0; i < ImageHeight; i++) {
		for (int j = 0; j < ImageWidth; j++) {
			int index = i * ImageWidth + j;
			/* 像素运算 */
			img[index].r = img[index].r >> 1;
			img[index].g = img[index].g >> 1;
			img[index].b = img[index].b >> 1;


			/* 像素运算结束 */
		}
	}
	return 0;
}

RGB* load_bmp(const char* filename)
{
	FILE* f_in = fopen(filename, "rb");

	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, f_in);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, f_in);

	ImageHeight = infoHeader.biHeight;
	ImageWidth = infoHeader.biWidth;
	int LineByteCnt = (((ImageWidth * infoHeader.biBitCount) + 31) >> 5) << 2;
	//int ImageDataSize = LineByteCnt * ImageHeight;
	PaddingSize = 4 - ((ImageWidth * infoHeader.biBitCount) >> 3) & 3;
	pad = (BYTE*)malloc(sizeof(BYTE) * PaddingSize);
	RGB* img = (RGB*)malloc(sizeof(RGB) * ImageHeight * ImageWidth);

	if (infoHeader.biBitCount == 24) {
		//Read Bitmp
		for (int i = 0; i < ImageHeight; i++) {
			for (int j = 0; j < ImageWidth; j++) {
				int index = i * ImageWidth + j;
				fread(&img[index], sizeof(RGB), 1, f_in);
				//img[index] = imgOpr24(img[index]); //反色运算
			}
			if (PaddingSize != 0)
			{
				fread(pad, 1, PaddingSize, f_in);
			}
		}
	}
	else
	{
		printf("此程序不支持非24位图片");
		return NULL;
	}

	fclose(f_in);
	return img;
}

void save_bmp(const char* filename, RGB* img)
{
	FILE* f_out = fopen(filename, "wb");
	fwrite(&fileHeader, sizeof(fileHeader), 1, f_out);
	fwrite(&infoHeader, sizeof(infoHeader), 1, f_out);
	for (int i = 0; i < ImageHeight; i++) {
		for (int j = 0; j < ImageWidth; j++)
			fwrite(&img[i * ImageWidth + j], sizeof(RGB), 1, f_out);
		if (PaddingSize != 0)
		{
			fwrite(pad, 1, PaddingSize, f_out);
		}
	}
	fclose(f_out);
	return;
}