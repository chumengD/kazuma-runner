#include "main.h"


int main() {
    initgraph(WINDOW_WID, WINDOW_HEI);

	// 手动将窗口居中（部分系统上 CW_USEDEFAULT 不会居中）
	HWND hWnd = GetHWnd();
	int screenW = GetSystemMetrics(SM_CXSCREEN);
	int screenH = GetSystemMetrics(SM_CYSCREEN);
	RECT rect;
	GetWindowRect(hWnd, &rect);
	int winW = rect.right - rect.left;
	int winH = rect.bottom - rect.top;
	SetWindowPos(hWnd, NULL,
		(screenW - winW) / 2,
		(screenH - winH) / 2,
		0, 0, SWP_NOSIZE | SWP_NOZORDER);

	setbkcolor(BROWN);
	cleardevice();
	//设置文字透明背景
	setbkmode(TRANSPARENT);
	InitResource();

	int result = 1;
	while (result) {
		result = menu();
	}
    closegraph();
    return 0;
}