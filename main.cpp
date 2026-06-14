#include "main.h"


int main() {
    initgraph(WINDOW_WID,WINDOW_HEI);
	setbkcolor(BROWN);
	cleardevice();
	//设置文字透明背景
	setbkmode(TRANSPARENT);
	InitMenu();

	int result = 1;
	while (result) {
		result = menu();
	}
    closegraph();
    return 0;
}