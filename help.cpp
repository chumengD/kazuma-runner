#include "main.h"

// ========================================
// 帮助页面
// ========================================
void HelpPage() {
    cleardevice();

    // 背景
    putimage(0, 0, &bg2);

    // 标题
    settextstyle(36, 0, _T("微软雅黑"));
    setbkmode(TRANSPARENT);
    setcolor(RGB(255, 255, 255));
    RECT tr = { 0, 20, WINDOW_WID, 65 };
    drawtext(_T("游戏帮助"), &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 分隔线
    setcolor(RGB(200, 200, 200));
    line(80, 75, WINDOW_WID - 80, 75);

    // 内容
    settextstyle(20, 0, _T("微软雅黑"));
    setcolor(RGB(240, 240, 240));

    const TCHAR* lines[] = {
        _T("> 游戏说明"),
        _T(""),
        _T("  空格键 / 上键  让角色跳跃，躲避障碍物"),
        _T("  下键            让角色蹲下，通过高处障碍"),
        _T("  ESC            暂停游戏"),
        _T("  R              游戏结束后重新开始"),
        _T(""),
        _T("> 游戏规则"),
        _T(""),
        _T("  每成功躲避一个障碍物 +1 分"),
        _T("  每获得 100 分，游戏速度会逐渐加快"),
        _T("  碰到障碍物游戏结束"),
        _T("  挑战最高分吧！"),
        _T(""),
        _T("> 提示"),
        _T(""),
        _T("  障碍物有不同类型，选择合适的躲避方式"),
        _T("  善用跳跃和下蹲的组合通过难关"),
        _T("  跳跃时会自动取消下蹲状态"),
    };

    int y = 90;
    for (int i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        outtextxy(60, y, lines[i]);
        y += 26;
    }

    // 底部提示
    settextstyle(18, 0, _T("微软雅黑"));
    setcolor(RGB(180, 180, 180));
    RECT br = { 0, 460, WINDOW_WID, 485 };
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
