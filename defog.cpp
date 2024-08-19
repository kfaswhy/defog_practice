#include "defog.h"

BITMAPFILEHEADER fileHeader;
BITMAPINFOHEADER infoHeader;
int height = 0;
int width = 0;
int PaddingSize = 0;
BYTE* pad = NULL;


//暗通道过滤参数
U32 dark_related_mask = 0;//暗通道最小值过滤：相对mask尺寸，若为0则使用固定大小
U32 dark_fixed_mask = 5; //暗通道最小值过滤：固定mask大小

//大气光计算
float light_ratio = 0.9;//大气光缩放

//透射系数
float omega = 0.8; //去雾强度
int kernel_size = 9; //透射系数高斯卷积：核大小，0表示不做滤波
float sigma = 111; //透射系数高斯卷积：方差
int diff_thd0 = 100; // 透射系数高斯卷积：邻像素差高于此值时，权重为0；
int diff_thd1 = 30; // 透射系数高斯卷积：邻像素差低于此值时，权重为1；
 
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

int img_process(RGB* img)
{

	//计算暗通道
	RGB* img_dark = (RGB*)malloc(sizeof(RGB) * height * width);
	calc_dark_chanel(img, img_dark);
	char bmp_dark[] = "C:/Work/Desktop/2_dark.bmp";
	save_bmp(bmp_dark, img_dark);

	//暗通道最小值滤波
	calc_min_filtered(img_dark);
	char bmp_dark_filtered[] = "C:/Work/Desktop/3_dark_filtered.bmp";
	save_bmp(bmp_dark_filtered, img_dark);

	//暗通道高斯滤波
	//calc_gauss_filtered(img_dark);
	//char bmp_dark_gauss[] = "C:/Work/Desktop/4_dark_gauss.bmp";
	//save_bmp(bmp_dark_gauss, img_dark);

	//估算大气光
	RGB light = calc_atmos_light(img, img_dark);


	//估算透射系数
	float* trans = (float*)malloc(sizeof(float) * height * width);
	calc_trans(img, trans, light);


	//恢复图像
	RGB* img_rec = (RGB*)malloc(sizeof(RGB) * height * width);
	recover_img(img, img_rec, trans, light);
	char bmp_recover[] = "C:/Work/Desktop/6_recover.bmp";
	save_bmp(bmp_recover, img_rec);


	return 0;
}


RGB calc_atmos_light(RGB* img, RGB* img_dark)
{
	U32 max_dark = 0;
	int max_i = 0;
	int max_j = 0;
	U8 max_rgb = 0;
	float ratio = 0;
	RGB light = { 0 };
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int index = i * width + j;
			if (img_dark[index].r > max_dark)
			{
				max_i = i;
				max_j = j;
				max_dark = img_dark[index].r;
			}
		}
	}

	int index = max_i * width + max_j;

	max_rgb = calc_max(max_rgb, img[index].r);
	max_rgb = calc_max(max_rgb, img[index].g);
	max_rgb = calc_max(max_rgb, img[index].b);
	max_rgb = calc_min(max_rgb, U8MAX);

	ratio = (float)U8MAX / max_rgb;
	ratio = calc_min(ratio, light_ratio);

	light.r = img[index].r * ratio;
	light.g = img[index].g * ratio;
	light.b = img[index].b * ratio;



	LOG("pos=[%d,%d], max_dark=%u, light=[%u,%u,%u]",
		height - max_i - 1, max_j, max_dark, light.r, light.g, light.b);

	return light;
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

int calc_min_filtered(RGB* img)
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

	RGB* filtered = (RGB*)malloc(sizeof(RGB) * height * width);

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
					//double r = 0;
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
	memcpy(img, filtered, sizeof(RGB) * height * width);
	free(filtered);
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

float calc_Interpolation(int x0, int x1, int y0, int y1, int x)
{
	if (x <= x0)
	{
		return y0;
	}
	else if (x >= x1)
	{
		return y1;
	}
	else
	{
		return ((float)(y1 - y0) * (x - x0) / (x1 - x0) + y0);
	}
}

float calc_distance(RGB* p1, RGB* p2)
{
	U16 tmp = 0;
	U16 sum = 0;
	float diff = 0;
	tmp = calc_abs((p1->r - p2->r));
	sum += (tmp*tmp);
	tmp = calc_abs((p1->g - p2->g));
	sum += (tmp * tmp);
	tmp = calc_abs((p1->b - p2->b));
	sum += (tmp * tmp);
	diff = sqrt((float)sum);
	if (sum != 0)
	{
		//LOG("p1 = [%u,%u,%u], p2 = [%u,%u,%u], sum = %u, diff = %f.", \
			p1->r, p1->g, p1->b, \
			p2->r, p2->g, p2->b, sum,diff);
	}
	return diff;
}

int calc_gauss_filtered(RGB* img)
{
	int x, y, i, j;

	RGB* filtered = (RGB*)malloc(sizeof(RGB) * height * width);
	if (kernel_size == 0)
	{
		//memcpy(filtered, img, sizeof(RGB) * height * width);
		return 0;
	}

	int half_size = ((float)kernel_size) / 2;
	float* kernel = (float*)malloc(kernel_size * kernel_size * sizeof(float));
	if (!kernel) {
		return -1; // Memory allocation failure
	}
	create_gaussian_kernel(kernel, kernel_size, sigma);

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			float r_sum = 0.0f, g_sum = 0.0f, b_sum = 0.0f;
			float weight_sum = 0;
			RGB *center = &img[y * width + x];
			for (i = -half_size; i <= half_size; i++) {
				for (j = -half_size; j <= half_size; j++) {
					int x_offset = x + j;
					int y_offset = y + i;

					x_offset = calc_max(x_offset, 0);
					x_offset = calc_min(x_offset, width - 1);
					y_offset = calc_max(y_offset, 0);
					y_offset = calc_min(y_offset, height - 1);

					RGB* pixel = &img[y_offset * width + x_offset];

					float diff = calc_distance(center, pixel);
					float ratio = calc_Interpolation(diff_thd1, diff_thd0, 1, 0, diff);
					if (ratio < 0.5)
					{
						//LOG("ratio = %f.", ratio);
					}
					float weight = kernel[(i + half_size) * kernel_size + (j + half_size)] * ratio;
					weight_sum += weight;
					r_sum += pixel->r * weight;
					g_sum += pixel->g * weight;
					b_sum += pixel->b * weight;
				}
			}

			filtered[y * width + x].r = (BYTE)roundf(r_sum / weight_sum);
			filtered[y * width + x].g = (BYTE)roundf(g_sum / weight_sum);
			filtered[y * width + x].b = (BYTE)roundf(b_sum / weight_sum);
		}
		print_prog(y, height);
	}

	memcpy(img, filtered, sizeof(RGB) * height * width);

	free(kernel);
	free(filtered);
	LOG("done.");
	return 0; // Success
}

int calc_trans(RGB* img, float* trans, RGB light)
{
	float tmp = 0.0;
	for (int y = 0; y < height; y++) 
	{
		for (int x = 0; x < width; x++) 
		{
			U32 index = y * width + x;
			float trans_tmp = U8MAX;
			RGB *cur = &img[index];
			tmp = (float)cur->r / calc_max(light.r, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			tmp = (float)cur->g / calc_max(light.g, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			tmp = (float)cur->b / calc_max(light.b, 1);
			trans_tmp = calc_min(trans_tmp, tmp);

			trans_tmp = 1.0 - omega * trans_tmp;
			//trans_tmp *= 255;

			trans[index]= trans_tmp;
		}
	}

	RGB* trans_dump = (RGB*)malloc(sizeof(RGB) * height * width);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			U32 index = y * width + x;
			trans_dump[index].r = (BYTE)calc_min(calc_max(trans[index] * U8MAX, 0), U8MAX);
			trans_dump[index].g = trans_dump[index].r;
			trans_dump[index].b = trans_dump[index].r;
		}
	}

	char bmp_trans_dump[] = "C:/Work/Desktop/4_trans_dump.bmp";
	save_bmp(bmp_trans_dump, trans_dump);

	calc_gauss_filtered(trans_dump);

	char bmp_trans_gauss[] = "C:/Work/Desktop/5_trans_gauss.bmp";
	save_bmp(bmp_trans_gauss, trans_dump);

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			U32 index = y * width + x;
			trans[index] = clp_range(0.0, (float)trans_dump[index].r / U8MAX, 1.0);
		}
	}

	free(trans_dump);
	LOG("done.");
	return 0;
}

void recover_img(RGB* img, RGB* img_rec, float* trans, RGB light)
{
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			U32 index = y * width + x;
			float t = calc_min(calc_max(trans[index], 0.001), 1);

			img[index].r = calc_max(img[index].r - light.r * (1 - t),0);


			img_rec[index].r = (BYTE)clp_range(0, (float)img[index].r / t, U8MAX);
			img[index].g = calc_max(img[index].g - light.g * (1 - t), 0);


			img_rec[index].g = (BYTE)clp_range(0, (float)img[index].g / t, U8MAX);
			img[index].b = calc_max(img[index].b - light.b * (1 - t), 0);

			img_rec[index].b = (BYTE)clp_range(0, (float)img[index].b / t, U8MAX);

			/*if (x == 0)
			{
				LOG("pos=%u,%u, t=%f, img=%u,%u,%u, rec=%u,%u,%u.", x, y,t,
					img[index].r, img[index].g, img[index].b,
					img_rec[index].r, img_rec[index].g, img_rec[index].b);
			}*/
		}
	}
	LOG("done.");
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