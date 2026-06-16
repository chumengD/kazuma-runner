#include "menu.h"

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


// 生成遮罩图：透明区域→白色（保留背景），内容区域→黑色（开出洞来）
void GenerateMask(IMAGE* src, IMAGE* mask) {
	int w = src->getwidth();
	int h = src->getheight();
	mask->Resize(w, h);

	DWORD* srcBuf = GetImageBuffer(src);
	DWORD* maskBuf = GetImageBuffer(mask);

	if (srcBuf && maskBuf) {
		int total = w * h;
		for (int i = 0; i < total; i++) {
			// EasyX IMAGE 缓冲格式: 0xAARRGGBB，透明/黑色像素的低 24 位为 0
			if ((srcBuf[i] & 0x00FFFFFF) == 0) {
				maskBuf[i] = 0x00FFFFFF; // 白色 → SRCAND 时保留背景
			}
			else {
				maskBuf[i] = 0x00000000; // 黑色 → SRCAND 时挖出洞
			}
		}
	}
}

IMAGE bg1, bg2, button, arrow, button_mask, arrow_mask, black_bg;
IMAGE rankStyle, rank_mask;


// 初始化菜单资源
void InitResource() {
	loadimage(&bg1, _T("public/bg/transform.png"));
	loadimage(&bg2, _T("public/bg2.png"));
	loadimage(&button, _T("public/button.png"));
	loadimage(&arrow, _T("public/arrow.png"));
	loadimage(&black_bg, _T("public/black_bg.png"));

	// 加载排行榜资源
	loadimage(&rankStyle, _T("public/rank.png"));
	if (rankStyle.getwidth() > 0) {
		GenerateMask(&rankStyle, &rank_mask);
	}

	// 为按钮和箭头生成遮罩图
	GenerateMask(&button, &button_mask);
	GenerateMask(&arrow, &arrow_mask);
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

// 绘制菜单·
void DrawMenu(bool isSelect) {
	cleardevice();
	putimage(0, 0, &bg1);
	settextstyle(28, 0, L"微软雅黑");
	settextcolor(BLACK);

	// 绘制菜单项
	int btn_wid = button.getwidth();
	int btn_hei = button.getheight();
	int btn_init_posx = (WINDOW_WID - btn_wid) / 2;
	int btn_init_posy = 20 + btn_hei;
	for (int i = 1; i <= MENU_COUNT; i++) {
		int btn_posx = btn_init_posx;
		int btn_posy = btn_init_posy + i * 75;
		// 透明贴图：遮罩 SRCAND 挖洞 + 原图 SRCPAINT 填充
		putimage(btn_posx, btn_posy, &button_mask, SRCAND);
		putimage(btn_posx, btn_posy, &button, SRCPAINT);

		// 箭头（仅当前选中项显示）
		if (i == currentMenu) {
			putimage(btn_posx - 40, btn_posy + 10, &arrow_mask, SRCAND);
			putimage(btn_posx - 40, btn_posy + 10, &arrow, SRCPAINT);
		}

		// 文字（每个按钮都画）
		RECT r = {
			btn_posx,
			btn_posy,
			btn_posx + btn_wid,
			btn_posy + btn_hei,
		};
		drawtext(
			menuItems[i-1],
			&r,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE
		);
	}
}

//处理玩家选中后的逻辑
int EnterMenu() {

	switch (currentMenu) {
	case 1:
		// 启动跑酷游戏，GameStart 阻塞直到用户退出
		return GameStart();

	case 2:
		drawRank();
		break;

	case 3:
		HelpPage();
		break;

	case 4:
		AboutPage();
		break;

	case 5:
		// 退出程序
		return 0;
	}
	return 1;
}
