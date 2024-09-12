#include "defog.h"

BITMAPFILEHEADER fileHeader;
BITMAPINFOHEADER infoHeader;
int height = 0;
int width = 0;


int PaddingSize = 0;
BYTE* pad = NULL;

//缩放参数
int sampling_related_ratio = 0; //缩放图像时，若此值不为0，则按比例缩放；若为0，则按下面两个参数缩放
int h_samp = 0;
int w_samp = 0;

//总增益
float iso = 1.3;

//暗通道过滤参数
U32 dark_related_mask = 0;//暗通道最小值过滤：相对mask尺寸，若为0则使用固定大小
U32 dark_fixed_mask = 5; //暗通道最小值过滤：固定mask大小

//大气光计算
float light_ratio = 1.0;//大气光缩放

//透射系数
float omega = 0.5; //去雾强度
int kernel_size = 0; //透射系数高斯卷积：核大小，0表示不做滤波
float sigma = 111; //透射系数高斯卷积：方差
int diff_thd0 = 100; // 透射系数高斯卷积：邻像素差高于此值时，权重为0；
int diff_thd1 = 30; // 透射系数高斯卷积：邻像素差低于此值时，权重为1；

int color_process = 0;

//恢复图像
S32 wgt_size = 0;
S32 *wgt_dark = NULL;
S32 *wgt_str = NULL;

S32 value_size = 0;
S32* value = NULL;
S32* value_str = NULL;

S32 sat_size = 0;
S32* sat = NULL;
S32* sat_str = NULL;

int main()
{
	//下面是色彩转换测试

	RGB rgb = { 125,89,16 };
	HSV hsv = { 0 };
	t0 = clock();
	LOG("start.");
	for (int k = 0; k < 100000000; k++)
	{
		hsv = rgb2hsv2(rgb);
	}
	LOG("hsv = %d,%d,%d.", hsv.h, hsv.s, hsv.v);



	//char cfg_setting[] = "setting.config";
	//load_cfg(cfg_setting);
	//
	//char bmp_in[] = "C:/Work/Desktop/1.bmp";
	//RGB* img = NULL;

	//img = load_bmp(bmp_in);
	//
	////save_bmp(bmp_out, img);

	//if (sampling_related_ratio != 0)
	//{
	//	w_samp = width / sampling_related_ratio;
	//	h_samp = height / sampling_related_ratio;
	//}

	//img_process(img);


	t1 = clock();
	U32 d_t = t1 - t0;
	LOG("sum time = %.3f.", (float)d_t / 1000);



	return 0;
}


RGB* img_sampling(RGB* img, int w1, int h1, int w2, int h2, bool method) {
	// 分配新图像的空间
	RGB* new_img = (RGB*)malloc(w2 * h2 * sizeof(RGB));
	if (!new_img) return NULL; // 检查内存分配是否成功

	for (int y = 0; y < h2; y++) {
		for (int x = 0; x < w2; x++) {
			// 计算在原图像中的位置
			float src_x = (float)x * w1 / w2;
			float src_y = (float)y * h1 / h2;

			if (method == 0) { // 邻近值采样
				int nearest_x = (int)(src_x + 0.5);
				int nearest_y = (int)(src_y + 0.5);

				// 边界检查
				if (nearest_x >= w1) nearest_x = w1 - 1;
				if (nearest_y >= h1) nearest_y = h1 - 1;

				new_img[y * w2 + x] = img[nearest_y * w1 + nearest_x];
			}
			else { // 双线性插值
				int x1 = (int)src_x;
				int y1 = (int)src_y;
				int x2 = x1 + 1 < w1 ? x1 + 1 : x1;
				int y2 = y1 + 1 < h1 ? y1 + 1 : y1;

				float dx = src_x - x1;
				float dy = src_y - y1;

				RGB p1 = img[y1 * w1 + x1];
				RGB p2 = img[y1 * w1 + x2];
				RGB p3 = img[y2 * w1 + x1];
				RGB p4 = img[y2 * w1 + x2];

				new_img[y * w2 + x].r = (unsigned char)(
					p1.r * (1 - dx) * (1 - dy) +
					p2.r * dx * (1 - dy) +
					p3.r * (1 - dx) * dy +
					p4.r * dx * dy
					);
				new_img[y * w2 + x].g = (unsigned char)(
					p1.g * (1 - dx) * (1 - dy) +
					p2.g * dx * (1 - dy) +
					p3.g * (1 - dx) * dy +
					p4.g * dx * dy
					);
				new_img[y * w2 + x].b = (unsigned char)(
					p1.b * (1 - dx) * (1 - dy) +
					p2.b * dx * (1 - dy) +
					p3.b * (1 - dx) * dy +
					p4.b * dx * dy
					);
			}
		}
		print_prog(y, h2);
	}

	return new_img;
}


int img_process(RGB* img)
{

	//img_gain(img);

	//缩放图片
	RGB* img_samp = NULL;
	img_samp = img_sampling(img, width, height, w_samp, h_samp, LINEAR);
	char bmp_samp[] = "C:/Work/Desktop/1_img_samp.bmp";
	save_bmp(bmp_samp, img_samp, w_samp, h_samp);
	
	//计算暗通道
	RGB* img_dark = (RGB*)malloc(sizeof(RGB) * w_samp * h_samp);
	calc_dark_chanel(img_samp, img_dark);
	char bmp_dark[] = "C:/Work/Desktop/2_dark.bmp";
	save_bmp(bmp_dark, img_dark, w_samp, h_samp); 

	//暗通道最小值滤波
	calc_min_filtered(img_dark);
	char bmp_dark_filtered[] = "C:/Work/Desktop/3_dark_filtered.bmp";
	save_bmp(bmp_dark_filtered, img_dark, w_samp, h_samp);

	//暗通道高斯滤波
	//calc_gauss_filtered(img_dark);
	//char bmp_dark_gauss[] = "C:/Work/Desktop/4_dark_gauss.bmp";
	//save_bmp(bmp_dark_gauss, img_dark);

	//估算大气光
	RGB light = calc_atmos_light(img_samp, img_dark);


	//估算透射系数
	float* trans = (float*)malloc(sizeof(float) * height * width);
	calc_trans(img_samp, trans, img_dark, light);


	//恢复图像
	RGB* img_rec = (RGB*)malloc(sizeof(RGB) * height * width);
	recover_img(img, img_rec, trans, light);

	char bmp_recover[] = "C:/Work/Desktop/7_recover.bmp";
	save_bmp(bmp_recover, img_rec, width, height);

	if (color_process == 1)
	{
		//调整饱和度
		set_color(img_rec);
		char bmp_color[] = "C:/Work/Desktop/8_color.bmp";
		save_bmp(bmp_color, img_rec, width, height);
	}

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
	for (int i = h_samp * 2 / 3; i < h_samp; i++)
	{
		for (int j = 0; j < w_samp; j++)
		{
			int index = i * w_samp + j;
			if (img_dark[index].r > max_dark)
			{
				max_i = i;
				max_j = j;
				max_dark = img_dark[index].r;
			}
		}
	}

	int index = max_i * w_samp + max_j;

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
		h_samp - max_i - 1, max_j, max_dark, light.r, light.g, light.b);

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

int img_gain(RGB* img)
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int index = i * width + j;
			/* 像素运算 */
			img[index].r = clp_range(0, img[index].r * iso, U8MAX);
			img[index].g = clp_range(0, img[index].g * iso, U8MAX);
			img[index].b = clp_range(0, img[index].b * iso, U8MAX);


			/* 像素运算结束 */
		}
	}
	return 0;
}

HSV rgb2hsv(RGB rgb) {
	int r = rgb.r;
	int g = rgb.g;
	int b = rgb.b;

	int max = r > g ? (r > b ? r : b) : (g > b ? g : b);
	int min = r < g ? (r < b ? r : b) : (g < b ? g : b);
	int delta = max - min;

	HSV hsv;
	hsv.v = max;

	if (delta == 0) {
		hsv.h = 0;
		hsv.s = 0;
		return hsv;
	}

	hsv.s = delta * 255 / max;

	if (r == max) {
		hsv.h = 60 * ((g - b) / (float)delta);
	}
	else if (g == max) {
		hsv.h = 60 * (2 + (b - r) / (float)delta);
	}
	else {
		hsv.h = 60 * (4 + (r - g) / (float)delta);
	}

	if (hsv.h < 0) {
		hsv.h += 360;
	}

	return hsv;
}

HSV rgb2hsv2(RGB rgb) {
	int r = rgb.r;
	int g = rgb.g;
	int b = rgb.b;

	int max = r > g ? (r > b ? r : b) : (g > b ? g : b);
	int min = r < g ? (r < b ? r : b) : (g < b ? g : b);
	int delta = max - min;
	int half_delta = (delta + 1) >> 1;

	HSV hsv;
	hsv.v = max;

	if (delta == 0) {
		hsv.h = 0;
		hsv.s = 0;
		return hsv;
	}

	hsv.s = 255 - ((min << 8) - min + (max + 1) >> 1) / max;

	int d120 = 120 * delta;
	int d240 = d120 << 1;


	if (r == max) {
		//hsv.h = 60 * ((g - b) / (float)delta);
		hsv.h = (60 * (g - b) + half_delta) / delta;
	}
	else if (g == max) {
		hsv.h = (60 * (b - r) + half_delta + d120) / delta;
	}
	else {
		hsv.h = (60 * (r - g) + half_delta + d240) / delta;
	}

	if (hsv.h < 0) {
		hsv.h += 360;
	}

	return hsv;
}



RGB hsv2rgb(HSV hsv)
{
	RGB rgb = { 0 };
	int h = (int)(hsv.h / 60) % 6;
	double f = hsv.h / 60.0 - h;
	double p = hsv.v * (1 - hsv.s);
	double q = hsv.v * (1 - f * hsv.s);
	double t = hsv.v * (1 - (1 - f) * hsv.s);

	switch (h) {
	case 0: rgb.r = hsv.v * 255; rgb.g = t * 255; rgb.b = p * 255; break;
	case 1: rgb.r = q * 255; rgb.g = hsv.v * 255; rgb.b = p * 255; break;
	case 2: rgb.r = p * 255; rgb.g = hsv.v * 255; rgb.b = t * 255; break;
	case 3: rgb.r = p * 255; rgb.g = q * 255; rgb.b = hsv.v * 255; break;
	case 4: rgb.r = t * 255; rgb.g = p * 255; rgb.b = hsv.v * 255; break;
	case 5: rgb.r = hsv.v * 255; rgb.g = p * 255; rgb.b = q * 255; break;
	}

	return rgb;
}


int set_color(RGB* img) {
	RGB* p_img = &img[0];
	for (int i = 0; i < height; i++) {
#pragma omp parallel
		for (int j = 0; j < width; j++) {
			
			HSV hsv = rgb2hsv(*p_img);

			int v_ratio = calc_interpolation_array(value, value_str, value_size, (int)(hsv.v * 100));
			hsv.v = hsv.v * v_ratio / 100;

			int s_ratio = calc_interpolation_array(sat, sat_str, sat_size, (int)(hsv.s * 100));
			hsv.s = hsv.s * s_ratio / 100;
			
			*p_img = hsv2rgb(hsv);
			p_img++;
		}
		print_prog(i, height);
	}
	LOG("Done.");
	return 0;
}

float fast_sqrt(float number) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y = number;
	i = *(long*)&y;                       // 将 float 解释为 long 类型
	i = 0x5f3759df - (i >> 1);            // 魔术数字
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));  // 近似值调整

	return 1.0f / y;
}

int calc_dark_chanel(RGB* img, RGB* img_dark)
{
	for (int i = 0; i < h_samp; i++) {
		for (int j = 0; j < w_samp; j++) {
			int index = i * w_samp + j;
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

	RGB* filtered = (RGB*)malloc(sizeof(RGB) * h_samp * w_samp);

	// 遍历图像中的每个像素
	for (int y = 0; y < h_samp; y++) {
		for (int x = 0; x < w_samp; x++) {
			int rgb_min = U8MAX;

			// 遍历滤波掩膜区域
			for (int ky = -half_mask; ky <= half_mask; ky++) {
				for (int kx = -half_mask; kx <= half_mask; kx++) {
					int yy = y + ky;
					int xx = x + kx;
					//U8 r = calc_abs(kx) + calc_abs(ky);
					//double r = 0;
					if (calc_abs(kx) < half_mask && calc_abs(ky) < half_mask && yy >= 0 && yy < height && xx >= 0 && xx < width)
					{
						int index = yy * w_samp + xx;
						rgb_min = calc_min(rgb_min, img[index].r);
						rgb_min = calc_max(rgb_min, U8MIN);
					}
				}
			}

			// 将最小值存储到过滤后的图像中
			filtered[y * w_samp + x].r = rgb_min;
			filtered[y * w_samp + x].g = rgb_min;
			filtered[y * w_samp + x].b = rgb_min;
		}
		print_prog(y, h_samp);
	}
	memcpy(img, filtered, sizeof(RGB) * h_samp * w_samp);
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

S32 calc_interpolation_array(S32* array_x, S32* array_y, S32 size, S32 x) 
{	
	if (x <= array_x[0]) {
		return array_y[0];
	}
	if (x >= array_x[size - 1]) {
		return array_y[size - 1];
	}
	for (U32 i = 0; i < size - 1; ++i) {
		if (x == array_x[i]) {
			return array_y[i];
		}
		if (x > array_x[i] && x < array_x[i + 1]) {
			S32 x0 = array_x[i];
			S32 x1 = array_x[i + 1];
			S32 y0 = array_y[i];
			S32 y1 = array_y[i + 1];
			return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
		}
	}

	// This should not be reached if input x is within the bounds
	return array_y[0];
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

U32 calc_distance(RGB* p1, RGB* p2)
{
	U16 tmp = 0;
	U32 sum = 0;
	float diff = 0;

	U16 d_r = calc_abs((p1->r - p2->r));
	sum += (d_r * d_r);
	U16 d_g = calc_abs((p1->g - p2->g));
	sum += (d_g * d_g);
	U16 d_b = calc_abs((p1->b - p2->b));
	sum += (d_b * d_b);
	//diff = fast_sqrt((float)sum);

	//if (sum != 0)
	//{
	//	LOG("p1 = [%u,%u,%u], p2 = [%u,%u,%u], sum = %u, diff = [%.2f,%.2f].", \
	//		p1->r, p1->g, p1->b, \
	//		p2->r, p2->g, p2->b, sum,diff, diff_fast);
	//}
	return sum;
}

int calc_gauss_filtered(RGB* img)
{
	int x, y, i, j;

	RGB* filtered = (RGB*)malloc(sizeof(RGB) * h_samp * w_samp);
	if (kernel_size == 0)
	{
		//memcpy(filtered, img, sizeof(RGB) * height * w_samp);
		return 0;
	}

	int half_size = ((float)kernel_size) / 2;
	float* kernel = (float*)malloc(kernel_size * kernel_size * sizeof(float));
	if (!kernel) {
		return -1; // Memory allocation failure
	}
	//create_gaussian_kernel(kernel, kernel_size, sigma);

	for (y = 0; y < h_samp; y++) {
		for (x = 0; x < w_samp; x++) {
			U64 r_sum = 0, g_sum = 0, b_sum = 0;
			U64 weight_sum = 0;
			RGB *center = &img[y * w_samp + x];
			for (i = -half_size; i <= half_size; i++) {
				for (j = -half_size; j <= half_size; j++) {
					int x_offset = x + j;
					int y_offset = y + i;

					x_offset = calc_max(x_offset, 0);
					x_offset = calc_min(x_offset, w_samp - 1);
					y_offset = calc_max(y_offset, 0);
					y_offset = calc_min(y_offset, h_samp - 1);

					RGB* pixel = &img[y_offset * w_samp + x_offset];

					U32 sum = calc_distance(center, pixel);

					

					const U32 thd1 = diff_thd1 * diff_thd1;
					const U32 thd0 = diff_thd0 * diff_thd0;

					U8 ratio = 0;
					if (sum <= thd1)
					{
						ratio = 100;
					}
					else if (sum >= thd0)
					{
						ratio = 0;
					}
					else 
					{
						float diff = fast_sqrt((float)sum); 
						ratio = calc_Interpolation(diff_thd1, diff_thd0, 100, 0, diff);
					}

					U32 weight = ratio;
					weight_sum += weight;
					r_sum += pixel->r * weight;
					g_sum += pixel->g * weight;
					b_sum += pixel->b * weight;
				}
			}

			filtered[y * w_samp + x].r = (BYTE)roundf(r_sum / weight_sum);
			filtered[y * w_samp + x].g = (BYTE)roundf(g_sum / weight_sum);
			filtered[y * w_samp + x].b = (BYTE)roundf(b_sum / weight_sum);
		}
		print_prog(y, h_samp);
	}

	memcpy(img, filtered, sizeof(RGB) * h_samp * w_samp);

	free(kernel);
	free(filtered);
	LOG("done.");
	return 0; // Success
}

int calc_trans(RGB* img, float* trans, RGB* img_dark, RGB light)
{
	float tmp = 0.0;
	
	//计算img图的透射率
	for (int y = 0; y < h_samp; y++) 
	{
		for (int x = 0; x < w_samp; x++)
		{
			U32 index = y * w_samp + x;
			float trans_tmp = U8MAX;
			RGB *cur = &img[index];
			tmp = (float)cur->r / calc_max(light.r, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			tmp = (float)cur->g / calc_max(light.g, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			tmp = (float)cur->b / calc_max(light.b, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			float defog_str = (float)calc_interpolation_array(wgt_dark, wgt_str, wgt_size, img_dark[index].r) / 100;
			trans_tmp = 1.0 - clp_range(0, omega * defog_str, 1) * trans_tmp;
			//trans_tmp *= 255;

			trans[index]= trans_tmp;
		}
	}
	//透射率转为rgb图
	RGB* trans_dump = (RGB*)malloc(sizeof(RGB) * h_samp * w_samp);
	for (int y = 0; y < h_samp; y++)
	{
		for (int x = 0; x < w_samp; x++)
		{
			U32 index = y * w_samp + x;
			trans_dump[index].r = (BYTE)calc_min(calc_max(trans[index] * U8MAX, 0), U8MAX);
			trans_dump[index].g = trans_dump[index].r;
			trans_dump[index].b = trans_dump[index].r;
		}
	}

	char bmp_trans_dump[] = "C:/Work/Desktop/4_trans_dump.bmp";
	save_bmp(bmp_trans_dump, trans_dump, w_samp, h_samp);
	//calc_gauss_filtered(trans_dump);
	//calc_gauss_filtered(trans_dump);
	//calc_gauss_filtered(trans_dump);
	calc_gauss_filtered(trans_dump);


	//透射率图双边滤波
	char bmp_trans_gauss[] = "C:/Work/Desktop/5_trans_gauss.bmp";
	save_bmp(bmp_trans_gauss, trans_dump, w_samp, h_samp);


	//透射率图放大至原始大小
	RGB* trans_rec = NULL;
	trans_rec = img_sampling(trans_dump, w_samp, h_samp, width, height, LINEAR);
	char bmp_trans_rec[] = "C:/Work/Desktop/6_trans_rec.bmp";
	save_bmp(bmp_trans_rec, trans_rec, width, height);


	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			U32 index = y * width + x;
			trans[index] = clp_range(0.0, (float)trans_rec[index].r / U8MAX, 1.0);
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
			float defog_str = 0.0;

			//defog_str = (float)calc_interpolation_array(wgt_dark, wgt_str, sizeof(wgt_dark) / sizeof(S32), img_dark[index].r) / 100;
		
			U32 tmp = 0;

			float t = calc_min(calc_max(trans[index], 0.001), 1);
			tmp = calc_max(img[index].r * iso - light.r * (1 - t), 0);
			img_rec[index].r = (BYTE)clp_range(0, (float)tmp / t, U8MAX);
			
			tmp = calc_max(img[index].g * iso - light.g * (1 - t), 0);
			img_rec[index].g = (BYTE)clp_range(0, (float)tmp / t, U8MAX);
			
			tmp = calc_max(img[index].b * iso - light.b * (1 - t), 0);
			img_rec[index].b = (BYTE)clp_range(0, (float)tmp / t, U8MAX);

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

void save_bmp(const char* filename, RGB* img, int width, int height)
{
	FILE* f_out = fopen(filename, "wb");

	fileHeader.bfType = 0x4D42; // 'BM'
	fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	fileHeader.bfSize = fileHeader.bfOffBits + width * height * sizeof(RGB);
	fileHeader.bfReserved1 = 0;
	fileHeader.bfReserved2 = 0;

	infoHeader.biSize = sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth = width;
	infoHeader.biHeight = height;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 24;
	infoHeader.biCompression = 0;
	infoHeader.biSizeImage = width * height * sizeof(RGB);
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

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

S32 load_cfg(const char* filename)
{
	char buffer[10240];
	S32 length = 0;
	S32 array_size = 0;
	cJSON* item = NULL;
	S32 i = 0;

	CONFIG_JSON config_json = { 0 };

	FILE *f_config = fopen(filename, "r");
	if (f_config == NULL)
	{
		LOG("Open setting.config ERROR."); // 如果打开文件失败，输出错误信息
		return ERROR;
	}

	length = fread(buffer, 1, sizeof(buffer), f_config);

	config_json.root = cJSON_Parse(buffer); // 将缓冲区中的JSON字符串解析成cJSON数据结构
	if (config_json.root == NULL)
	{
		LOG("Config analysis ERROR."); // 如果解析JSON失败，输出错误信息
		return ERROR;
	}

	config_json.sampling_related_ratio = cJSON_GetObjectItem(config_json.root, "sampling_related_ratio");
	sampling_related_ratio = config_json.sampling_related_ratio->valueint;
	//LOG("sampling_related_ratio: %d", sampling_related_ratio);

	config_json.sampling_height = cJSON_GetObjectItem(config_json.root, "sampling_height");
	h_samp = config_json.sampling_height->valueint;
	//LOG("sampling_height: %d", sampling_height);

	config_json.sampling_width = cJSON_GetObjectItem(config_json.root, "sampling_width");
	w_samp = config_json.sampling_width->valueint;
	//LOG("sampling_width: %d", sampling_width);

	



	config_json.iso = cJSON_GetObjectItem(config_json.root, "iso");
	if (config_json.iso != NULL && cJSON_IsNumber(config_json.iso))
	{
		iso = config_json.iso->valuedouble;
		//LOG("iso: %f", iso);
	}
	else
	{
		LOG("missing param: iso.");
	}

	config_json.dark_related_mask = cJSON_GetObjectItem(config_json.root, "dark_related_mask");
	if (config_json.dark_related_mask != NULL && cJSON_IsNumber(config_json.dark_related_mask))
	{
		dark_related_mask = config_json.dark_related_mask->valueint;
		//LOG("dark_related_mask: %d", dark_related_mask);
	}
	else
	{
		LOG("missing param: dark_related_mask.");
	}

	config_json.dark_fixed_mask = cJSON_GetObjectItem(config_json.root, "dark_fixed_mask");
	if (config_json.dark_fixed_mask != NULL && cJSON_IsNumber(config_json.dark_fixed_mask))
	{
		dark_fixed_mask = config_json.dark_fixed_mask->valueint;
		//LOG("dark_fixed_mask: %d", dark_fixed_mask);
	}
	else
	{
		LOG("missing param: dark_fixed_mask.");
	}

	config_json.light_ratio = cJSON_GetObjectItem(config_json.root, "light_ratio");
	if (config_json.light_ratio != NULL && cJSON_IsNumber(config_json.light_ratio))
	{
		light_ratio = config_json.light_ratio->valuedouble;
		//LOG("light_ratio: %f", light_ratio);
	}
	else
	{
		LOG("missing param: light_ratio.");
	}

	config_json.omega = cJSON_GetObjectItem(config_json.root, "omega");
	if (config_json.omega != NULL && cJSON_IsNumber(config_json.omega))
	{
		omega = config_json.omega->valuedouble;
		//LOG("omega: %f", omega);
	}
	else
	{
		LOG("missing param: omega.");
	}

	config_json.kernel_size = cJSON_GetObjectItem(config_json.root, "kernel_size");
	if (config_json.kernel_size != NULL && cJSON_IsNumber(config_json.kernel_size))
	{
		kernel_size = config_json.kernel_size->valueint;
		//LOG("kernel_size: %d", kernel_size);
	}
	else
	{
		LOG("missing param: kernel_size.");
	}

	config_json.sigma = cJSON_GetObjectItem(config_json.root, "sigma");
	if (config_json.sigma != NULL && cJSON_IsNumber(config_json.sigma))
	{
		sigma = config_json.sigma->valueint;
		//LOG("sigma: %d", sigma);
	}
	else
	{
		LOG("missing param: sigma.");
	}

	config_json.diff_thd0 = cJSON_GetObjectItem(config_json.root, "diff_thd0");
	diff_thd0 = config_json.diff_thd0->valueint;
	//LOG("diff_thd0: %d", diff_thd0);

	config_json.diff_thd1 = cJSON_GetObjectItem(config_json.root, "diff_thd1");
	diff_thd1 = config_json.diff_thd1->valueint;
	//LOG("diff_thd1: %d", diff_thd1);


	config_json.color_process = cJSON_GetObjectItem(config_json.root, "color_process");
	color_process = config_json.color_process->valueint;
	//LOG("color_process: %d", color_process);

	config_json.wgt_dark = cJSON_GetObjectItem(config_json.root, "wgt_dark");
	array_size = cJSON_GetArraySize(config_json.wgt_dark);
	wgt_size = array_size;
	wgt_dark = (S32*)malloc(array_size * sizeof(S32));
	for (i = 0; i < array_size; i++)
	{
		item = cJSON_GetArrayItem(config_json.wgt_dark, i);
		if (item != NULL && cJSON_IsNumber(item))
		{
			wgt_dark[i] = item->valueint;
			//LOG("wgt_dark[%d]: %d", i, wgt_dark[i]);
		}
	}

	config_json.wgt_str = cJSON_GetObjectItem(config_json.root, "wgt_str");
	array_size = cJSON_GetArraySize(config_json.wgt_str);
	wgt_str = (S32*)malloc(array_size * sizeof(S32));
	for (i = 0; i < array_size; i++)
	{
		item = cJSON_GetArrayItem(config_json.wgt_str, i);
		if (item != NULL && cJSON_IsNumber(item))
		{
			wgt_str[i] = item->valueint;
			//LOG("wgt_str[%d]: %d", i, wgt_str[i]);
		}
	}


	config_json.value = cJSON_GetObjectItem(config_json.root, "value");
	array_size = cJSON_GetArraySize(config_json.value);
	value_size = array_size;
	value = (S32*)malloc(array_size * sizeof(S32));
	for (i = 0; i < array_size; i++)
	{
		item = cJSON_GetArrayItem(config_json.value, i);
		if (item != NULL && cJSON_IsNumber(item))
		{
			value[i] = item->valueint;
			//LOG("value[%d]: %d", i, value[i]);
		}
	}

	config_json.value_str = cJSON_GetObjectItem(config_json.root, "value_str");
	array_size = cJSON_GetArraySize(config_json.value_str);
	value_str = (S32*)malloc(array_size * sizeof(S32));
	for (i = 0; i < array_size; i++)
	{
		item = cJSON_GetArrayItem(config_json.value_str, i);
		if (item != NULL && cJSON_IsNumber(item))
		{
			value_str[i] = item->valueint;
			//LOG("value_str[%d]: %d", i, value_str[i]);
		}
	}

	config_json.sat = cJSON_GetObjectItem(config_json.root, "sat");
	array_size = cJSON_GetArraySize(config_json.sat);
	sat_size = array_size;
	sat = (S32*)malloc(array_size * sizeof(S32));
	for (i = 0; i < array_size; i++)
	{
		item = cJSON_GetArrayItem(config_json.sat, i);
		if (item != NULL && cJSON_IsNumber(item))
		{
			sat[i] = item->valueint;
			//LOG("sat[%d]: %d", i, sat[i]);
		}
	}

	config_json.sat_str = cJSON_GetObjectItem(config_json.root, "sat_str");
	array_size = cJSON_GetArraySize(config_json.sat_str);
	sat_str = (S32*)malloc(array_size * sizeof(S32));
	for (i = 0; i < array_size; i++)
	{
		item = cJSON_GetArrayItem(config_json.sat_str, i);
		if (item != NULL && cJSON_IsNumber(item))
		{
			sat_str[i] = item->valueint;
			//LOG("sat_str[%d]: %d", i, sat_str[i]);
		}
	}
}