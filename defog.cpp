#include "defog.h"

BITMAPFILEHEADER fileHeader;
BITMAPINFOHEADER infoHeader;

int height = 0;
int width = 0;
int PaddingSize = 0;
BYTE* pad = NULL;



U32 dark_related_mask = 0;//暗通道最小值过滤：相对mask尺寸，若为0则使用固定大小
U32 dark_fixed_mask = 10; //暗通道最小值过滤：固定mask大小

int kernel_size = 11; //暗通道高斯卷积：核大小
float sigma = 2; //暗通道高斯卷积：方差


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

void print_prog(U32 cur_pos, U32 tgt)
{
	end = clock();
	
	if ((end - start) >= 1000)
	{
		LOG("Processing: %d%%.", cur_pos * 100 / tgt);
		start = clock();
	}
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
			int rgb_min = U8MAX;
			rgb_min = calc_min(rgb_min, img[index].r);
			rgb_min = calc_min(rgb_min, img[index].g);
			rgb_min = calc_min(rgb_min, img[index].b);
			rgb_min = calc_max(rgb_min, U8MIN);
			img_dark[index].r = rgb_min;
			img_dark[index].g = rgb_min;
			img_dark[index].b = rgb_min;
			/* 像素运算结束 */
		}
	}
	LOG("done.");

	return 0;
}

int calc_min_filtered(RGB* img, RGB* filtered)
{
	U32 mask = 0;
	if (dark_related_mask != 0)
	{
		if (width > height)
		{
			mask = height / dark_related_mask;
		}
		else
		{
			mask = width / dark_related_mask;
		}
	}
	else
	{
		mask = dark_fixed_mask;
	}


	int half_mask = mask / 2;
	// 遍历图像中的每个像素
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int rgb_min = U8MAX;

			// 遍历滤波掩膜区域
			for (int ky = -half_mask; ky <= half_mask; ky++) {
				for (int kx = -half_mask; kx <= half_mask; kx++) {
					int yy = y + ky;
					int xx = x + kx;
					double r = sqrt(kx * kx + ky * ky);
					if (r < half_mask && yy >= 0 && yy < height && xx >= 0 && xx < width)
					{
						int index = yy * width + xx;
						rgb_min = calc_min(rgb_min, img[index].r);
						rgb_min = calc_max(rgb_min, U8MIN);
					}
				}
			}

			// 将最小值存储到过滤后的图像中
			filtered[y * width + x].r = rgb_min;
			filtered[y * width + x].g = rgb_min;
			filtered[y * width + x].b = rgb_min;
		}
		print_prog(y, height);
	}

	LOG("done.");

	return 0; // 返回 0 表示成功
}

void create_gaussian_kernel(float* kernel, int kernel_size, float sigma) {
	int i, j;
	float sum = 0.0f;
	int half_size = ((float)kernel_size)/ 2;
	float sigma2 = 2.0f * sigma * sigma;

	for (i = 0; i < kernel_size; i++) {
		for (j = 0; j < kernel_size; j++) {
			int x = i - half_size;
			int y = j - half_size;
			int tmp = x * x + y * y;
			kernel[i * kernel_size + j] = exp(-tmp / sigma2);
			sum += kernel[i * kernel_size + j];
		}
	}

	// Normalize the kernel
	for (i = 0; i < kernel_size; i++) 
	{
		for (j = 0; j < kernel_size; j++)
		{
			kernel[i * kernel_size + j] /= sum;
			//printf("%.2f, ", kernel[i * kernel_size + j]);
		}
		//printf("\n");
	}
}

int calc_gauss_filtered(RGB* img, RGB* filtered) {
	int x, y, i, j;
	int half_size = ((float)kernel_size) / 2;
	float* kernel = (float*)malloc(kernel_size * kernel_size * sizeof(float));
	if (!kernel) {
		return -1; // Memory allocation failure
	}
	create_gaussian_kernel(kernel, kernel_size, sigma);

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			float r_sum = 0.0f, g_sum = 0.0f, b_sum = 0.0f;

			for (i = -half_size; i <= half_size; i++) {
				for (j = -half_size; j <= half_size; j++) {
					int x_offset = x + j;
					int y_offset = y + i;

					if (x_offset < 0) x_offset = 0;
					if (x_offset >= width) x_offset = width - 1;
					if (y_offset < 0) y_offset = 0;
					if (y_offset >= height) y_offset = height - 1;

					RGB* pixel = &img[y_offset * width + x_offset];
					float weight = kernel[(i + half_size) * kernel_size + (j + half_size)];

					r_sum += pixel->r * weight;
					g_sum += pixel->g * weight;
					b_sum += pixel->b * weight;
				}
			}

			filtered[y * width + x].r = (BYTE)roundf(r_sum);
			filtered[y * width + x].g = (BYTE)roundf(g_sum);
			filtered[y * width + x].b = (BYTE)roundf(b_sum);
		}
	}

	free(kernel);
	LOG("done.");
	return 0; // Success
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
	calc_min_filtered(img_dark, img_dark_filtered);
	char bmp_dark_filtered[] = "C:/Work/Desktop/3_dark_filtered.bmp";
	save_bmp(bmp_dark_filtered, img_dark_filtered);

	//暗通道低通滤波
	RGB* img_dark_gauss = (RGB*)malloc(sizeof(RGB) * height * width);
	calc_gauss_filtered(img_dark_filtered, img_dark_gauss);
	char bmp_dark_gauss[] = "C:/Work/Desktop/4_dark_gauss.bmp";
	save_bmp(bmp_dark_gauss, img_dark_gauss);


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