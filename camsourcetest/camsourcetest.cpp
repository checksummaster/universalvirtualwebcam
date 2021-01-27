#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib")

#include "camsource.h"
camsource _camsource;
BITMAPINFOHEADER bi;

int anim = 1;
int line = 0;



int _tmain()
{
	printf("Open cam\n");
	while (!_kbhit() && !_camsource.init() ) {}

	_camsource.fastget = false;

	while (!_kbhit()) {
		BYTE* buf = (BYTE *)_camsource.get(&bi);
		unsigned size = bi.biWidth * bi.biHeight * bi.biBitCount / 8;
		if (buf) {
			printf("%d %d %d\n", bi.biWidth, bi.biHeight, bi.biBitCount);
			switch (anim) {
			case 0:
				for (int i = 0; i < size; ++i) buf[i] = rand();
				break;

			case 1:
				memset(buf, 0, size);
				if (line >= bi.biHeight - 10) {
					line = 0;
				}
				memset(&buf[line++ * bi.biWidth * bi.biBitCount / 8], 0xFF, bi.biWidth * bi.biBitCount / 8 * 10);
				break;

			}
		}
		_camsource.release();
	}
	_getch();
}

