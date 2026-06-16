#include "main.h"

// ========================================
// 常量
// ========================================
#define GRAVITY 0.8
#define JUMP_VELOCITY -20
#define JUMP_RELEASE_VELOCITY -12  // 松开跳跃键时的最小上升速度（短按=小跳）
#define GROUND_Y 350
#define PLAYER_X 120
#define BASE_SPEED 5
#define MAX_SPEED 14
#define MAX_OBSTACLES 3
#define OBS_SCALE 90       // 障碍物碰撞盒 = 显示尺寸 × 60%
#define MIN_SPAWN_DELAY 80
#define MAX_SPAWN_DELAY 160
#define FRAME_DELAY 16       // ~60 FPS
#define FLYING_OBS_HIGH 220  // 飞行物高度-高位（头部）
#define FLYING_OBS_LOW  310  // 飞行物高度-低位（贴地）
#define FLYING_CHANCE 30     // 30% 概率生成飞行物
#define FLYING_SPEED_BONUS 3 // 飞行物额外速度

// 精灵显示尺寸（loadimage 时缩放至以下尺寸）
#define PLAYER_STAND_W   60   // kazuma 站立宽
#define PLAYER_STAND_H  145   // kazuma 站立高
#define PLAYER_CROUCH_W  60   // kazuma 下蹲宽
#define PLAYER_CROUCH_H  95   // kazuma 下蹲高

// ========================================
// 结构体
// ========================================
struct Player {
    int x, y;          // 精灵左上角坐标
    float vy;          // 垂直速度（float 以支持小数重力）
    int w, h;          // 碰撞盒宽高
    bool jumping;
    bool crouching;
    int animFrame;     // 动画帧 (0/1)
    int animTimer;     // 帧切换倒计时
};

struct Obstacle {
    int x, y;          // 绘制坐标（原始尺寸）
    int w, h;          // 碰撞盒宽高（缩放后）
    int drawW, drawH;  // 绘制宽高（原始尺寸）
    int type;          // 0 = c1, 1 = c2
    bool active;
    bool flying;       // true = 飞行物，需下蹲躲避
};

// ========================================
// 全局游戏状态 (文件作用域)
// ========================================
static struct {
    Player player;
    Obstacle obstacles[MAX_OBSTACLES];
    int score;
    int speed;
    int spawnTimer;
    bool over;
    bool paused;       // 暂停标志
    int frameCount;    // 总帧数，用于地面动画等
} gs;

// 已加载的精灵
static IMAGE img_player1;   // kazuma 站立
static IMAGE img_player2;   // kazuma 下蹲
static IMAGE img_obs1;      // lalatina 高障碍物
static IMAGE img_obs2;      // megume 矮宽障碍物
static IMAGE img_flying;    // aqura 飞行物
static bool g_imagesLoaded = false;

// 最高分（rang.cpp 通过 extern 引用）
int g_highScore = 0;

// 游戏中标志（menu.cpp 通过 extern 引用）
bool g_inGame = false;

// ========================================
// 前向声明
// ========================================
static void LoadGameImages();
static void InittatGameSe();
static void UpdateGame();
static void DrawGame();
static void HandleGameInput();
static void SpawnObstacle();
static bool CheckCollision(const Player& p, const Obstacle& o);
static void DrawPlayer();
static void DrawObstacles();
static void DrawGround();
static void DrawScore();
static void DrawPauseOverlay();
static void DrawGameOverOverlay();

// ========================================
// 图片加载（只加载一次）
// ========================================
static void LoadGameImages() {
    if (g_imagesLoaded) return;
    loadimage(&img_player1, _T("public/kazuma.png"),   PLAYER_STAND_W,  PLAYER_STAND_H,  true);
    loadimage(&img_player2, _T("public/kazuma.png"),   PLAYER_CROUCH_W, PLAYER_CROUCH_H, true);
    loadimage(&img_obs1,    _T("public/lalatina.png"), 48,              120,              true);
    loadimage(&img_obs2,    _T("public/megume.png"),   100,             58,              true);
    loadimage(&img_flying,  _T("public/aqura.png"),    100,             60,              true);
    g_imagesLoaded = true;
}

// ========================================
// 初始化 / 重置游戏状态
// ========================================
static void InitGameState() {
    srand((unsigned)time(NULL));

    // 玩家
    gs.player.x = PLAYER_X;
    gs.player.y = GROUND_Y - img_player1.getheight();
    gs.player.vy = 0;
    gs.player.w = img_player1.getwidth();
    gs.player.h = img_player1.getheight();
    gs.player.jumping = false;
    gs.player.crouching = false;
    gs.player.animFrame = 0;
    gs.player.animTimer = 6;

    // 障碍物
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        gs.obstacles[i].active = false;
        gs.obstacles[i].flying = false;
    }

    // 游戏状态
    gs.score = 0;
    gs.speed = BASE_SPEED;
    gs.spawnTimer = 50;
    gs.over = false;
    gs.paused = false;
    gs.frameCount = 0;
}

// ========================================
// 游戏逻辑更新
// ========================================
static void UpdateGame() {
    if (gs.over || gs.paused) return;

    Player& p = gs.player;
    gs.frameCount++;

    // ---- 跳跃物理 ----
    if (p.jumping) {
        p.y += p.vy;
        p.vy += GRAVITY;
        // 落地检测（始终以站立高度为准）
        int groundLevel = GROUND_Y - img_player1.getheight();
        if (p.y >= groundLevel) {
            p.y = groundLevel;
            p.jumping = false;
            p.vy = 0;
        }
    } else {
        // 地面状态：根据下蹲调整位置
        if (p.crouching) {
            p.y = GROUND_Y - img_player2.getheight();
            p.h = img_player2.getheight();
            p.w = img_player2.getwidth();
        } else {
            p.y = GROUND_Y - img_player1.getheight();
            p.h = img_player1.getheight();
            p.w = img_player1.getwidth();
        }
    }

    // ---- 移动障碍物 ----
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        Obstacle& o = gs.obstacles[i];
        if (!o.active) continue;

        o.x -= gs.speed + (o.flying ? FLYING_SPEED_BONUS : 0);

        // 离开左边界 → 得分
        if (o.x + o.w < 0) {
            o.active = false;
            gs.score++;

            // 每 20 分加速
            if (gs.score % 20 == 0 && gs.speed < MAX_SPEED) {
                gs.speed++;
            }
        }
    }

    // ---- 障碍物生成 ----
    gs.spawnTimer--;
    if (gs.spawnTimer <= 0) {
        SpawnObstacle();
        int minDelay = MIN_SPAWN_DELAY - gs.speed;
        int maxDelay = MAX_SPAWN_DELAY - gs.speed;
        if (minDelay < 40) minDelay = 40;
        if (maxDelay < 70) maxDelay = 70;
        gs.spawnTimer = minDelay + rand() % (maxDelay - minDelay + 1);
    }

    // ---- 碰撞检测 ----
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!gs.obstacles[i].active) continue;
        if (CheckCollision(p, gs.obstacles[i])) {
            gs.over = true;
            if (gs.score > g_highScore) {
                g_highScore = gs.score;
            }
            return;
        }
    }

    // ---- 角色动画 ----
    if (!p.jumping && !p.crouching) {
        p.animTimer--;
        if (p.animTimer <= 0) {
            p.animFrame = (p.animFrame + 1) % 2;
            p.animTimer = 6;
        }
    }
}

// ========================================
// AABB 碰撞检测（内缩碰撞盒，更公平）
// ========================================
static bool CheckCollision(const Player& p, const Obstacle& o) {
    int insetX = 8;
    int insetY = 5;

    int pL = p.x + insetX;
    int pR = p.x + p.w - insetX;
    int pT = p.y + insetY;
    int pB = p.y + p.h - insetY;

    int oL = o.x + insetX;
    int oR = o.x + o.w - insetX;
    int oT, oB;

    if (o.flying) {
        // 飞行物碰撞盒在空气中
        oT = o.y + insetY;
        oB = o.y + o.h - insetY;
        // 下蹲可躲避飞行物
        if (p.crouching) return false;
    } else {
        // 地面障碍物：碰撞盒底部贴地
        oB = GROUND_Y - insetY;
        oT = oB - o.h + insetY;
    }

    return (pR > oL && pL < oR && pB > oT && pT < oB);
}

// ========================================
// 生成新障碍物
// ========================================
static void SpawnObstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!gs.obstacles[i].active) {
            Obstacle& o = gs.obstacles[i];
            o.active = true;
            o.flying = (rand() % 100) < FLYING_CHANCE;

            if (o.flying) {
                // 飞行物：用 aqura 的尺寸
                o.type = -1;  // 飞行物无地面类型
                o.drawW = img_flying.getwidth();
                o.drawH = img_flying.getheight();
                o.w = o.drawW * OBS_SCALE / 100;
                o.h = o.drawH * OBS_SCALE / 100;
                o.x = WINDOW_WID + rand() % 80;
                o.y = (rand() % 2) ? FLYING_OBS_HIGH : FLYING_OBS_LOW;
            } else {
                // 地面障碍物：随机 lalatina (0) 或 megume (1)
                o.type = rand() % 2;
                if (o.type == 0) {
                    o.drawW = img_obs1.getwidth();
                    o.drawH = img_obs1.getheight();
                } else {
                    o.drawW = img_obs2.getwidth();
                    o.drawH = img_obs2.getheight();
                }
                o.w = o.drawW * OBS_SCALE / 100;
                o.h = o.drawH * OBS_SCALE / 100;
                o.x = WINDOW_WID + rand() % 80;
                o.y = GROUND_Y - o.drawH;    // 贴地
            }
            return;
        }
    }
}

// ========================================
// 键盘输入处理
// ========================================
static void HandleGameInput() {
    ExMessage msg;
    while (peekmessage(&msg, EX_KEY)) {
        // ---- 松开下键：取消下蹲（hold-to-crouch） ----
        if (msg.vkcode == VK_DOWN && msg.message == WM_KEYUP) {
            gs.player.crouching = false;
            continue;
        }

        // ---- 松开跳跃键：短按小跳（长按大跳） ----
        if ((msg.vkcode == VK_SPACE || msg.vkcode == VK_UP)
            && msg.message == WM_KEYUP) {
            Player& p = gs.player;
            if (p.jumping && p.vy < JUMP_RELEASE_VELOCITY) {
                p.vy = JUMP_RELEASE_VELOCITY;   // 削减上升速度 → 小跳
            }
            continue;
        }

        if (msg.message != WM_KEYDOWN) continue;

        // ---- ESC ----
        if (msg.vkcode == VK_ESCAPE) {
            if (gs.over || gs.paused) {
                // 游戏结束或暂停中 → 返回菜单
                g_inGame = false;
                return;
            }
            // 游戏中 → 暂停
            gs.paused = true;
            continue;
        }

        // ---- 暂停中的输入 ----
        if (gs.paused) {
            // 空格/上键/回车 取消暂停
            if (msg.vkcode == VK_SPACE || msg.vkcode == VK_UP ||
                msg.vkcode == VK_RETURN) {
                gs.paused = false;
            }
            continue;
        }

        // ---- 游戏结束后的输入 ----
        if (gs.over) {
            if (msg.vkcode == 'R' || msg.vkcode == 'r') {
                InitGameState();
                return;   // 立即返回，避免残留按键被当成游戏操作
            }
            continue;
        }

        // ---- 游戏中的操作 ----
        Player& p = gs.player;
        if (msg.vkcode == VK_SPACE || msg.vkcode == VK_UP) {
            if (!p.jumping) {
                p.jumping = true;
                p.crouching = false;
                p.vy = JUMP_VELOCITY;   // 按下跳跃：全速起跳
            }
        }
        if (msg.vkcode == VK_DOWN) {
            if (!p.jumping) {
                p.crouching = true;   // 按住下键保持下蹲
            }
        }
    }
}

// ========================================
// 渲染
// ========================================
static void DrawGame() {
    cleardevice();

    // 天空背景
    setfillcolor(RGB(135, 206, 235));
    solidrectangle(0, 0, WINDOW_WID, GROUND_Y);

    DrawGround();
    DrawObstacles();
    DrawPlayer();
    DrawScore();

    if (gs.paused) {
        DrawPauseOverlay();
    }
    if (gs.over) {
        DrawGameOverOverlay();
    }
}

// ========================================
// 绘制角色
// ========================================
static void DrawPlayer() {
    const Player& p = gs.player;
    IMAGE* img = p.crouching ? &img_player2 : &img_player1;
    putimage(p.x, p.y, img);
}

// ========================================
// 绘制障碍物
// ========================================
static void DrawObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        const Obstacle& o = gs.obstacles[i];
        if (!o.active) continue;

        IMAGE* img;
        if (o.flying) {
            img = &img_flying;                   // aqura 飞行物
            putimage(o.x, o.y, img);
        } else {
            img = (o.type == 0) ? &img_obs1 : &img_obs2;
            putimage(o.x, GROUND_Y - o.drawH, img);  // 地面障碍物贴地
        }
    }
}

// ========================================
// 绘制地面 + 滚动虚线
// ========================================
static void DrawGround() {
    // 地面底色
    setfillcolor(RGB(139, 90, 43));
    solidrectangle(0, GROUND_Y, WINDOW_WID, WINDOW_HEI);

    // 地面线
    setcolor(RGB(100, 60, 30));
    setlinestyle(PS_SOLID, 3);
    line(0, GROUND_Y, WINDOW_WID, GROUND_Y);

    // 滚动虚线（视差效果）
    int offset = (gs.frameCount * (gs.speed - 2)) % 40;
    if (offset < 0) offset += 40;

    setcolor(RGB(160, 120, 60));
    setlinestyle(PS_SOLID, 2);
    for (int x = -offset; x < WINDOW_WID; x += 40) {
        line(x,     GROUND_Y + 10, x + 18, GROUND_Y + 10);
        line(x + 12, GROUND_Y + 25, x + 30, GROUND_Y + 25);
    }
}

// ========================================
// 绘制分数
// ========================================
static void DrawScore() {
    settextstyle(20, 0, _T("Consolas"));
    setbkmode(TRANSPARENT);

    TCHAR buf[64];

    setcolor(RGB(40, 40, 40));
    _stprintf_s(buf, _T("得分: %d"), gs.score);
    outtextxy(20, 15, buf);

    _stprintf_s(buf, _T("最高分: %d"), g_highScore);
    outtextxy(WINDOW_WID - 170, 15, buf);

    // 存活时间 = 帧数 × 每帧时长
    int totalSec = gs.frameCount * FRAME_DELAY / 1000;
    int min = totalSec / 60;
    int sec = totalSec % 60;
    setcolor(RGB(40, 40, 40));
    _stprintf_s(buf, _T("时间: %02d:%02d"), min, sec);
    outtextxy(20, 42, buf);

    setcolor(RGB(80, 80, 80));
    _stprintf_s(buf, _T("速度: %d"), gs.speed);
    outtextxy(20, 69, buf);
}

// ========================================
// 绘制暂停界面
// ========================================
static void DrawPauseOverlay() {
    // 半透明遮罩效果：画一个深色矩形
    setfillcolor(RGB(20, 20, 20));
    solidrectangle(0, 0, WINDOW_WID, WINDOW_HEI);

    settextstyle(36, 0, _T("微软雅黑"));
    setbkmode(TRANSPARENT);
    setcolor(RGB(255, 255, 255));

    RECT r = { 0, 145, WINDOW_WID, 205 };
    drawtext(_T("暂停中"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    settextstyle(18, 0, _T("微软雅黑"));
    setcolor(RGB(200, 200, 200));

    r = { 0, 230, WINDOW_WID, 265 };
    drawtext(_T("按 空格/回车 继续 | 按 ESC 返回菜单"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ========================================
// 绘制游戏结束界面
// ========================================
static void DrawGameOverOverlay() {
    // 遮罩
    setfillcolor(RGB(30, 30, 30));
    solidrectangle(0, 0, WINDOW_WID, WINDOW_HEI);

    // GAME OVER
    settextstyle(48, 0, _T("Consolas"));
    setbkmode(TRANSPARENT);
    setcolor(RGB(255, 60, 60));
    RECT r = { 0, 85, WINDOW_WID, 155 };
    drawtext(_T("GAME OVER"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 分数
    settextstyle(26, 0, _T("微软雅黑"));
    setcolor(RGB(255, 255, 255));
    TCHAR buf[64];
    _stprintf_s(buf, _T("得分: %d"), gs.score);
    r = { 0, 165, WINDOW_WID, 200 };
    drawtext(buf, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 存活时间
    {
        int totalSec = gs.frameCount * FRAME_DELAY / 1000;
        int min = totalSec / 60;
        int sec = totalSec % 60;
        _stprintf_s(buf, _T("存活: %02d:%02d"), min, sec);
        r = { 0, 205, WINDOW_WID, 240 };
        drawtext(buf, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 新纪录
    if (gs.score >= g_highScore && gs.score > 0) {
        setcolor(RGB(255, 215, 0));
        settextstyle(22, 0, _T("微软雅黑"));
        r = { 0, 255, WINDOW_WID, 290 };
        drawtext(_T("** 新纪录! **"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 操作提示
    settextstyle(20, 0, _T("微软雅黑"));
    setcolor(RGB(200, 200, 200));
    r = { 0, 345, WINDOW_WID, 380 };
    drawtext(_T("按 R 键重新开始 | 按 ESC 返回菜单"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ========================================
// GameStart — 游戏入口（供菜单调用）
// 返回 1 表示返回菜单继续，返回 0 表示退出程序
// ========================================
int GameStart() {
    LoadGameImages();
    InitGameState();
    g_inGame = true;
    BeginBatchDraw();          // 开启双缓冲，消除闪烁

    while (g_inGame) {
        HandleGameInput();
        if (!g_inGame) break;   // ESC 在暂停/结束时触发退出

        // 暂停或游戏结束：只渲染，不更新逻辑
        if (gs.paused || gs.over) {
            DrawGame();
            FlushBatchDraw();
            Sleep(FRAME_DELAY);
            continue;
        }

        // 正常游戏
        UpdateGame();
        DrawGame();
        FlushBatchDraw();
        Sleep(FRAME_DELAY);
    }

    EndBatchDraw();            // 退出游戏循环，关闭双缓冲
    g_inGame = false;
    return 1;   // 返回菜单（真正的退出在菜单中选"退出游戏"）
}
