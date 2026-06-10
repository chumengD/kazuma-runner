#include "main.h"

//声明函数
//void EnterMenu();
//void DrawMenu(bool isSelect=false);

//1-5
int currentMenu = 1;
const int MENU_COUNT = 5;
const wchar_t* menuItems[] =
{
	L"1.继续游戏",
	L"2.排行榜",
	L"3.帮助",
	L"4.关于",
	L"5.退出游戏"
};

// 菜单背景图
IMAGE bg1, bg2, button, arrow;

// 初始化菜单资源
void InitMenu() {
	loadimage(&bg1, _T("public/bg1.png"));
	loadimage(&bg2, _T("public/bg2.png"));
	loadimage(&button, _T("public/button.png"));
	loadimage(&arrow, _T("public/arrow.png"));
}

// 读取键盘输入
int menu() {
	ExMessage msg;

	// 先画一次初始菜单
	DrawMenu();

	int result;
	while (true) {
		if (peekmessage(&msg, EX_KEY)) {
			if (msg.message == WM_KEYDOWN) {
				// 再次按下 ESC 退出菜单
				switch (msg.vkcode) {
				case VK_ESCAPE:
					return 0;
				case VK_UP:
					if (currentMenu > 1) currentMenu--;
					break;
				case VK_DOWN:
					if (currentMenu < MENU_COUNT) currentMenu++;
					break;
				case VK_RETURN:
					result = EnterMenu();
					return result;
				}
				DrawMenu();
			}
		}
	}
	return 0;
}

// 绘制菜单
void DrawMenu(bool isSelect = false) {
	cleardevice();
	putimage(0, 0, &bg1);
	settextstyle(28, 0, L"微软雅黑");

	// 绘制菜单项
	int btn_wid = button.getwidth();
	int btn_hei = button.getheight();
	int btn_init_posx = (WINDOW_WID - btn_wid) / 2;
	int btn_init_posy = 20 + btn_hei;
	for (int i = 0; i < MENU_COUNT; i++) {
		int btn_posx = btn_init_posx;
		int btn_posy = btn_init_posy + i * 75;
		// 绘制按钮
		putimage(btn_posx, btn_posy, &button);

		// 箭头（仅当前选中项显示）
		if (i == currentMenu) {
			putimage(btn_posx - 40, btn_posy + 10, &arrow);
		}

		// 文字（每个按钮都画）
		RECT r = {
			btn_posx,
			btn_posy,
			btn_posx + btn_wid,
			btn_posy + btn_hei,
		};
		drawtext(
			menuItems[i],
			&r,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE
		);
	}
}

int EnterMenu() {
	//处理玩家选中后的逻辑
	switch (currentMenu) {
	case 1: 
		//game_start();
		break;

	case 2:
		//rang()
		break;

	case 3:
		//help()
		break;
		
	case 4:
		//about()
		break;

	case 5:
		exit(0);
	}
}
