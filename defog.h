#ifndef _DEFOG_H_
#define _DEFOG_H_

#include <iostream>
#include <windows.h> 
#include <time.h>
#include "cJSON.h"

#define U64 unsigned long long
#define U32 unsigned int
#define S32 int
#define U16 unsigned short
#define U8 unsigned char 
#define U16MAX (0xFFFF)
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

U32 t0;
U32 t1;

typedef struct _RGB
{
	BYTE b;
	BYTE g;
	BYTE r;
}RGB;

typedef struct {
	double h;  
	double s;
	double v;
}HSV;

typedef struct
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
}RGBA;

/* Ω‚ŒˆJSON∏Ò Ω */
typedef struct _CONFIG_JSON
{
	cJSON* root;

	cJSON* iso;
	cJSON* dark_related_mask;
	cJSON* dark_fixed_mask;
	cJSON* light_ratio;
	cJSON* omega;
	cJSON* kernel_size;
	cJSON* sigma;
	cJSON* diff_thd0;
	cJSON* diff_thd1;
	cJSON* wgt_dark;
	cJSON* wgt_str;
	cJSON* sat;
	cJSON* sat_str;
	cJSON* value;
	cJSON* value_str;

}CONFIG_JSON;


int main();

void print_prog(U32 cur_pos, U32 tgt);

int img_gain(RGB* img);

int set_color(RGB* img);

float fast_sqrt(float number);

RGB calc_atmos_light(RGB* img, RGB* img_dark);


int calc_dark_chanel(RGB* img, RGB* img_dark);

int calc_min_filtered(RGB* img);

void create_gaussian_kernel(float* kernel, int kernel_size, float sigma);

S32 calc_interpolation_array(S32* array_x, S32* array_y, S32 size, S32 x);

float calc_Interpolation(int x0, int x1, int y0, int y1, int x);

U32 calc_distance(RGB* p1, RGB* p2);

int calc_gauss_filtered(RGB* img);

int calc_trans(RGB* img, float* trans, RGB* img_dark, RGB light);
void recover_img(RGB* img, RGB* img_rec, float* trans, RGB light);

int img_process(RGB* img);

RGB* load_bmp(const char* filename);

void save_bmp(const char* filename, RGB* img);

S32 load_cfg(const char* filename);

#endif