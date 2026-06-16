#include "main.h"

// ========================================
// 关于页面
// ========================================
void AboutPage() {
    cleardevice();

    // 背景
    putimage(0, 0, &bg1);
    putimage(100, 100, &black_bg);

    // 标题
    settextstyle(36, 0, _T("微软雅黑"));
    setbkmode(TRANSPARENT);
    

    // 分隔线
    setcolor(RGB(200, 200, 200));
    line(80, 75, WINDOW_WID - 80, 75);

    // 内容
    settextstyle(20, 0, _T("微软雅黑"));
    setcolor(RGB(240, 240, 240));

    const TCHAR* lines[] = {
        _T("和真快跑 (Kazuma Runner)"),
        _T(""),
        _T("版本: 1.0"),
        _T(""),
        _T("基于 EasyX 图形库开发的"),
        _T("横版跑酷游戏。"),
        _T(""),
        _T("控制角色跳跃和下蹲"),
        _T("躲避障碍物，挑战最高分！"),
        _T(""),
        _T("开发: chumengD，ShiYiShang-Bin"),
        _T(""),
        _T("技术栈: C++20 + EasyX"),
        _T(""),
        _T("(C) 2026 Kazuma Runner"),
    };

    int y = 110;
    for (int i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        outtextxy(110, y, lines[i]);
        y += 28;
    }

    // 底部提示
    settextstyle(18, 0, _T("微软雅黑"));
    setcolor(RGB(180, 180, 180));
    RECT br = { 0, 650, WINDOW_WID, WINDOW_HEI };
    drawtext(_T("按 ESC 返回菜单"), &br, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 等待 ESC
    ExMessage msg;
    while (true) {
        if (peekmessage(&msg, EX_KEY)) {
            if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {
                break;
            }
        }
        Sleep(50);
    }
}
