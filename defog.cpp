#include "defog.h"

BITMAPFILEHEADER fileHeader;
BITMAPINFOHEADER infoHeader;
int height = 0;
int width = 0;
int PaddingSize = 0;
BYTE* pad = NULL;

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
	t0 = clock();
	char cfg_setting[] = "setting.config";
	load_cfg(cfg_setting);
	
	char bmp_in[] = "C:/Work/Desktop/1.bmp";
	RGB* img = NULL;

	img = load_bmp(bmp_in);
	img_process(img);
	//save_bmp(bmp_out, img);
	t1 = clock();

	U32 d_t = t1 - t0;
	LOG("sum time = %.3f.", (float)d_t / 1000);
	return 0;
}
 


int img_process(RGB* img)
{

	//img_gain(img);

	//计算暗通道
	RGB* img_dark = (RGB*)malloc(sizeof(RGB) * height * width);
	calc_dark_chanel(img, img_dark);
	char bmp_dark[] = "C:/Work/Desktop/2_dark.bmp";
	save_bmp(bmp_dark, img_dark);

	//暗通道最小值滤波
	calc_min_filtered(img_dark);
	//calc_min_filtered(img_dark);
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
	calc_trans(img, trans, img_dark, light);


	//恢复图像
	RGB* img_rec = (RGB*)malloc(sizeof(RGB) * height * width);
	recover_img(img, img_rec, trans, light);

	char bmp_recover[] = "C:/Work/Desktop/6_recover.bmp";
	save_bmp(bmp_recover, img_rec);
	//调整饱和度
	set_color(img_rec);
	char bmp_color[] = "C:/Work/Desktop/7_color.bmp";
	save_bmp(bmp_color, img_rec);


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
	for (int i = height*2 / 3; i < height; i++)
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

HSV rgb2hsv(RGB rgb)
{
	HSV hsv = { 0 };
	double rd, gd, bd;
	double max, min, delta;

	rd = rgb.r / 255.0;
	gd = rgb.g / 255.0;
	bd = rgb.b / 255.0;

	max = fmax(fmax(rd, gd), bd);
	min = fmin(fmin(rd, gd), bd);
	delta = max - min;

	// 计算V值 (明度)
	hsv.v = max;

	// 计算S值 (饱和度)
	if (max == 0) {
		hsv.s = 0;
	}
	else {
		hsv.s = delta / max;
	}

	// 计算H值 (色相)
	if (delta == 0) {
		hsv.h = 0; // 当delta为0时，无色相
	}
	else {
		if (max == rd) {
			hsv.h = 60 * fmod(((gd - bd) / delta), 6);
		}
		else if (max == gd) {
			hsv.h = 60 * (((bd - rd) / delta) + 2);
		}
		else if (max == bd) {
			hsv.h = 60 * (((rd - gd) / delta) + 4);
		}

		if (hsv.h < 0) {
			hsv.h += 360;
		}
	}

	//printf("RGB(%d, %d, %d) -> HSV(%.2f, %.2f, %.2f)\n", rgb.r, rgb.g, rgb.b, hsv.h, hsv.s, hsv.v);
	return hsv;
}

RGB hsv2rgb(HSV hsv)
{
	RGB rgb = { 0 };
	double c = hsv.v * hsv.s;
	double x = c * (1 - fabs(fmod(hsv.h / 60.0, 2) - 1));
	double m = hsv.v - c;
	double r_, g_, b_;

	if (hsv.h >= 0 && hsv.h < 60) {
		r_ = c, g_ = x, b_ = 0;
	}
	else if (hsv.h >= 60 && hsv.h < 120) {
		r_ = x, g_ = c, b_ = 0;
	}
	else if (hsv.h >= 120 && hsv.h < 180) {
		r_ = 0, g_ = c, b_ = x;
	}
	else if (hsv.h >= 180 && hsv.h < 240) {
		r_ = 0, g_ = x, b_ = c;
	}
	else if (hsv.h >= 240 && hsv.h < 300) {
		r_ = x, g_ = 0, b_ = c;
	}
	else {
		r_ = c, g_ = 0, b_ = x;
	}

	rgb.r = (unsigned char)((r_ + m) * 255);
	rgb.g = (unsigned char)((g_ + m) * 255);
	rgb.b = (unsigned char)((b_ + m) * 255);

	return rgb;
}

int set_color(RGB* img) {
	RGB* p_img = &img[0];
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			
			HSV hsv = rgb2hsv(*p_img);

			float v_ratio = (float)calc_interpolation_array(value, value_str, value_size, (int)(hsv.v * 100)) / 100;
			hsv.v *= v_ratio;

			float s_ratio = (float)calc_interpolation_array(sat, sat_str, sat_size, (int)(hsv.s * 100)) / 100;
			hsv.s *= s_ratio;
			
			*p_img = hsv2rgb(hsv);
			p_img++;
		}
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
					//U8 r = calc_abs(kx) + calc_abs(ky);
					//double r = 0;
					if (calc_abs(kx) < half_mask && calc_abs(ky) < half_mask && yy >= 0 && yy < height && xx >= 0 && xx < width)
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
	//create_gaussian_kernel(kernel, kernel_size, sigma);

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			U64 r_sum = 0, g_sum = 0, b_sum = 0;
			U64 weight_sum = 0;
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

int calc_trans(RGB* img, float* trans, RGB* img_dark, RGB light)
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
			float defog_str = (float)calc_interpolation_array(wgt_dark, wgt_str, wgt_size, img_dark[index].r) / 100;
			trans_tmp = 1.0 - clp_range(0, omega * defog_str, 1) * trans_tmp;
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
	//calc_gauss_filtered(trans_dump);
	//calc_gauss_filtered(trans_dump);
	//calc_gauss_filtered(trans_dump);
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