#pragma once
#include "main.h"

// ---- 菜单内部函数 ----
int EnterMenu();
void DrawMenu(bool isSelect = false);

// ---- 菜单资源（定义在 menu.cpp） ----
extern IMAGE bg1, button, arrow;

// ---- rank.cpp 函数 ----
void drawRank();
