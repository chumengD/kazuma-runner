#include "main.h"

// ========================================
// 常量
// ========================================
#define GRAVITY 1
#define JUMP_VELOCITY -13
#define GROUND_Y 380
#define PLAYER_X 100
#define BASE_SPEED 5
#define MAX_SPEED 14
#define MAX_OBSTACLES 3
#define MIN_SPAWN_DELAY 35
#define MAX_SPAWN_DELAY 90
#define FRAME_DELAY 16   // ~60 FPS

// ========================================
// 结构体
// ========================================
struct Player {
    int x, y;          // 精灵左上角坐标
    int vy;            // 垂直速度
    int w, h;          // 碰撞盒宽高
    bool jumping;
    bool crouching;
    int animFrame;     // 动画帧 (0/1)
    int animTimer;     // 帧切换倒计时
};

struct Obstacle {
    int x, y;          // 精灵左上角坐标
    int w, h;          // 碰撞盒宽高
    int type;          // 0 = c1, 1 = c2
    bool active;
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
static IMAGE img_player1;   // b1
static IMAGE img_player2;   // b2
static IMAGE img_obs1;      // c1
static IMAGE img_obs2;      // c2
static bool g_imagesLoaded = false;

// 最高分（rang.cpp 通过 extern 引用）
int g_highScore = 0;

// 游戏中标志（menu.cpp 通过 extern 引用）
bool g_inGame = false;

// ========================================
// 前向声明
// ========================================
static void LoadGameImages();
static void InitGameState();
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
    loadimage(&img_player1, _T("public/b1.png"));
    loadimage(&img_player2, _T("public/b2.png"));
    loadimage(&img_obs1,    _T("public/c1.png"));
    loadimage(&img_obs2,    _T("public/c2.png"));
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

        o.x -= gs.speed;

        // 离开左边界 → 得分
        if (o.x + o.w < 0) {
            o.active = false;
            gs.score++;

            // 每 100 分加速
            if (gs.score % 100 == 0 && gs.speed < MAX_SPEED) {
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
        if (minDelay < 18) minDelay = 18;
        if (maxDelay < 35) maxDelay = 35;
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
    int oT = o.y + insetY;
    int oB = o.y + o.h - insetY;

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
            o.type = rand() % 2;

            if (o.type == 0) {
                o.w = img_obs1.getwidth();
                o.h = img_obs1.getheight();
            } else {
                o.w = img_obs2.getwidth();
                o.h = img_obs2.getheight();
            }

            o.x = WINDOW_WID + rand() % 80;
            o.y = GROUND_Y - o.h;
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
                p.vy = JUMP_VELOCITY;
            }
        }
        if (msg.vkcode == VK_DOWN) {
            if (!p.jumping) {
                p.crouching = !p.crouching;
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
    IMAGE* img;

    if (p.crouching) {
        img = &img_player2;
    } else {
        img = (p.animFrame == 0) ? &img_player1 : &img_player2;
    }

    putimage(p.x, p.y, img);
}

// ========================================
// 绘制障碍物
// ========================================
static void DrawObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        const Obstacle& o = gs.obstacles[i];
        if (!o.active) continue;

        IMAGE* img = (o.type == 0) ? &img_obs1 : &img_obs2;
        putimage(o.x, o.y, img);
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

    setcolor(RGB(80, 80, 80));
    _stprintf_s(buf, _T("速度: %d"), gs.speed);
    outtextxy(20, 42, buf);
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

    RECT r = { 0, 170, WINDOW_WID, 220 };
    drawtext(_T("暂停中"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    settextstyle(18, 0, _T("微软雅黑"));
    setcolor(RGB(200, 200, 200));

    r = { 0, 240, WINDOW_WID, 270 };
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
    RECT r = { 0, 130, WINDOW_WID, 190 };
    drawtext(_T("GAME OVER"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 分数
    settextstyle(26, 0, _T("微软雅黑"));
    setcolor(RGB(255, 255, 255));
    TCHAR buf[64];
    _stprintf_s(buf, _T("得分: %d"), gs.score);
    r = { 0, 210, WINDOW_WID, 245 };
    drawtext(buf, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 新纪录
    if (gs.score >= g_highScore && gs.score > 0) {
        setcolor(RGB(255, 215, 0));
        settextstyle(22, 0, _T("微软雅黑"));
        r = { 0, 255, WINDOW_WID, 285 };
        drawtext(_T("** 新纪录! **"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 操作提示
    settextstyle(20, 0, _T("微软雅黑"));
    setcolor(RGB(200, 200, 200));
    r = { 0, 340, WINDOW_WID, 370 };
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

    while (g_inGame) {
        HandleGameInput();
        if (!g_inGame) break;   // ESC 在暂停/结束时触发退出

        // 暂停或游戏结束：只渲染，不更新逻辑
        if (gs.paused || gs.over) {
            DrawGame();
            Sleep(FRAME_DELAY);
            continue;
        }

        // 正常游戏
        UpdateGame();
        DrawGame();
        Sleep(FRAME_DELAY);
    }

    g_inGame = false;
    return 1;   // 返回菜单（真正的退出在菜单中选"退出游戏"）
}
