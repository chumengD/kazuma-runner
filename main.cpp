#include "main.h"


int main() {
    initgraph(WINDOW_WID,WINDOW_HEI); 
	setbkcolor(BROWN);
	cleardevice();
	//设置文字透明背景
	setbkmode(TRANSPARENT);
	InitMenu();  
	ExMessage msg;

	int result = 1;
	while (result) {
		//game() 
		if (peekmessage(&msg, EX_KEY)) {
			if (msg.message == WM_KEYDOWN &&
				msg.vkcode == VK_ESCAPE) {
				result = menu();
			}
		}
	}
    closegraph();
    return 0;
}