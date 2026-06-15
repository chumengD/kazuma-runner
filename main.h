#pragma once
#include <graphics.h>
#include <iostream>
#include <conio.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>


#define WINDOW_WID 1200
#define WINDOW_HEI 700

using namespace std;

// ---- 菜单 ----
int menu();
void InitMenu();

// ---- 游戏核心 ----
int GameStart();

// ---- 菜单页面 ----
void HelpPage();
void AboutPage();
void LeaderboardPage();
void drawRank();


// ---- 菜单背景图（在 menu.cpp 中加载，其他文件 extern 引用） ----
extern IMAGE bg2;

// ---- 游戏状态标志（菜单判断是否正在游戏中） ----
extern bool g_inGame;
