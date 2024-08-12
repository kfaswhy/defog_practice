#include <iostream>
#include "defog.h"

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

