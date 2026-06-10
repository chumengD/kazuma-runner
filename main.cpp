#include "main.h"


int main() {
    initgraph(650, 500, EX_SHOWCONSOLE); // 加上 EX_SHOWCONSOLE 可以同时看到控制台窗口
    // ... 绘图代码 ...
    _getch();
    closegraph();
    return 0;
}