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

	// 设置窗口图标（显示在标题栏和任务栏）
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
	if (hIcon) {
		// 设置窗口实例图标（标题栏）
		SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		// 设置窗口类图标（任务栏使用类图标）
		SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR)hIcon);
		SetClassLongPtr(hWnd, GCLP_HICONSM, (LONG_PTR)hIcon);
	}

	setbkcolor(BROWN);
	cleardevice();
	//设置文字透明背景
	setbkmode(TRANSPARENT);
	InitResource();

	int result = 1;
	while (result) {
		result = menu();
	}
    stop_music();
    closegraph();
    return 0;
}