#include "defog.h"

BITMAPFILEHEADER fileHeader;
BITMAPINFOHEADER infoHeader;

int height = 0;
int width = 0;
int PaddingSize = 0;
BYTE* pad = NULL;

int mask_size = 9;

int main()
{
	char bmp_in[] = "C:/Work/Desktop/1.bmp";
	char bmp_out[] = "C:/Work/Desktop/1_out.bmp";
	RGB* img = NULL;
	img = load_bmp(bmp_in);
	img_process(img);
	//save_bmp(bmp_out, img);
	return 0;
}

int img_darken(RGB* img)
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int index = i * width + j;
			/* 像素运算 */
			img[index].r = img[index].r >> 1;
			img[index].g = img[index].g >> 0;
			img[index].b = img[index].b >> 1;


			/* 像素运算结束 */
		}
	}
	return 0;
}

int calc_dark_chanel(RGB* img, RGB* img_dark)
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int index = i * width + j;
			/* 像素运算 */
			int rgb_min = u8max;
			rgb_min = calc_min(rgb_min, img[index].r);
			rgb_min = calc_min(rgb_min, img[index].g);
			rgb_min = calc_min(rgb_min, img[index].b);
			rgb_min = calc_max(rgb_min, u8min);
			img_dark[index].r = rgb_min;
			img_dark[index].g = rgb_min;
			img_dark[index].b = rgb_min;
			/* 像素运算结束 */
		}
	}

	return 0;
}

int calc_dark_filtered(RGB* img_dark, RGB* filtered)
{
	int half_mask = mask_size / 2;
	// 遍历图像中的每个像素
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int rgb_min = u8max;

			// 遍历滤波掩膜区域
			for (int ky = -half_mask; ky <= half_mask; ky++) {
				for (int kx = -half_mask; kx <= half_mask; kx++) {
					int yy = y + ky;
					int xx = x + kx;

					// 检查边界条件
					if (yy >= 0 && yy < height && xx >= 0 && xx < width) {
						int index = yy * width + xx;
						rgb_min = calc_min(rgb_min, img_dark[index].r);
						rgb_min = calc_max(rgb_min, u8min);
					}
				}
			}

			// 将最小值存储到过滤后的图像中
			filtered[y * width + x].r = rgb_min;
			filtered[y * width + x].g = rgb_min;
			filtered[y * width + x].b = rgb_min;
		}
	}

	return 0; // 返回 0 表示成功
}

int img_process(RGB* img)
{
	//img_darken(img);
	//计算暗通道
	RGB* img_dark = (RGB*)malloc(sizeof(RGB) * height * width);
	calc_dark_chanel(img, img_dark);
	char bmp_dark[] = "C:/Work/Desktop/2_dark.bmp";
	save_bmp(bmp_dark, img_dark);

	//暗通道滤波
	RGB* img_dark_filtered = (RGB*)malloc(sizeof(RGB) * height * width);
	calc_dark_filtered(img_dark, img_dark_filtered);
	char bmp_dark_filtered[] = "C:/Work/Desktop/3_dark_filtered.bmp";
	save_bmp(bmp_dark_filtered, img_dark_filtered);
	return 0;
}

RGB* load_bmp(const char* filename)
{
	FILE* f_in = fopen(filename, "rb");

	fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, f_in);
	fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, f_in);

	height = infoHeader.biHeight;
	width = infoHeader.biWidth;
	int LineByteCnt = (((width * infoHeader.biBitCount) + 31) >> 5) << 2;
	//int ImageDataSize = LineByteCnt * height;
	PaddingSize = 4 - ((width * infoHeader.biBitCount) >> 3) & 3;
	pad = (BYTE*)malloc(sizeof(BYTE) * PaddingSize);
	RGB* img = (RGB*)malloc(sizeof(RGB) * height * width);

	if (infoHeader.biBitCount == 24) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				int index = i * width + j;
				fread(&img[index], sizeof(RGB), 1, f_in);
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
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++)
			fwrite(&img[i * width + j], sizeof(RGB), 1, f_out);
		if (PaddingSize != 0)
		{
			fwrite(pad, 1, PaddingSize, f_out);
		}
	}
	fclose(f_out);
	return;
}