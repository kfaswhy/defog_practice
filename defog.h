#ifndef _DEFOG_H_
#define _DEFOG_H_

#include <iostream>
#include <windows.h> 
#include <time.h>

#define U32 unsigned int
#define U16 unsigned short
#define U8 unsigned char 
#define U8MAX (255)
#define U8MIN (0)

#define M_PI 3.1415926

#define calc_min(a,b) ((a)>(b)?(b):(a))
#define calc_max(a,b) ((a)<(b)?(b):(a))
#define calc_abs(a) ((a)>0?(a):(-a))
#define clp_range(min,x,max) calc_min(calc_max((x), (min)), (max))

#define LOG(...) printf("%s [%d]: ", __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");


U32 start = clock();
U32 end;
U8 prog_print = 1;

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


int main();

void print_prog(U32 cur_pos, U32 tgt);

RGB calc_atmos_light(RGB* img, RGB* img_dark);

int img_darken(RGB* img);

int calc_dark_chanel(RGB* img, RGB* img_dark);

int calc_min_filtered(RGB* img);

void create_gaussian_kernel(float* kernel, int kernel_size, float sigma);

float calc_Interpolation(int x0, int x1, int y0, int y1, int x);

int calc_gauss_filtered(RGB* img);

int calc_trans(RGB* img, float* trans, RGB light);

void recover_img(RGB* img, RGB* img_rec, float* trans, RGB light);

int img_process(RGB* img);

RGB* load_bmp(const char* filename);

void save_bmp(const char* filename, RGB* img);


#endif