// TTTest.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "svpng.h"
void test_rgba(void) {
	unsigned char rgba[256 * 256 * 4], *p = rgba;
	unsigned x, y;
	FILE* fp = NULL;
	fopen_s(&fp, "rgba.png", "wb");
	for (y = 0; y < 256; y++)
		for (x = 0; x < 256; x++) {
			*p++ = (unsigned char)x;                /* R */
			*p++ = (unsigned char)y;                /* G */
			*p++ = 128;                             /* B */
			*p++ = (unsigned char)((x + y) / 2);    /* A */
		}
	svpng(fp, 256, 256, rgba, 1);
	fclose(fp);
}

int main()
{
	test_rgba();
	getchar();
    return 0;
}

