#include "main.h"

// ========================================
// 常量
// ========================================
#define GRAVITY 0.8f
#define JUMP_VELOCITY -20
#define JUMP_RELEASE_VELOCITY -12  // 松开跳跃键时的最小上升速度（短按=小跳）
#define GROUND_Y 450
#define PLAYER_X 120
#define BASE_SPEED 5
#define MAX_SPEED 14
#define MAX_OBSTACLES 3
#define OBS_SCALE 80      
#define MIN_SPAWN_DELAY 80
#define MAX_SPAWN_DELAY 160
#define FRAME_DELAY 16       // ~60 FPS
#define FLYING_OBS_HIGH 265  // 飞行物高度-高位（头部）
#define FLYING_OBS_LOW  340  // 飞行物高度-低位（贴地）
#define FLYING_CHANCE 30     // 30% 概率生成飞行物
#define FLYING_SPEED_BONUS 3 // 飞行物额外速度

// ---- 滚动背景 ----
#define SKY_TOP_COLOR     RGB(80, 150, 240)
#define SKY_BOTTOM_COLOR  RGB(180, 220, 255)
#define CLOUD_COLOR       RGB(255, 255, 255)

#define HILL_FAR_COLOR    RGB(55, 125, 55)
#define HILL_NEAR_COLOR   RGB(75, 165, 75)
#define HILL_BASE_Y       (GROUND_Y - 30)
#define HILL_FAR_AMPL     55
#define HILL_NEAR_AMPL    35
#define HILL_FAR_SPEED    0.06
#define HILL_NEAR_SPEED   0.12
#define HILL_TILE_W       800

#define GRASS_STRIP_H     35

// 精灵显示尺寸（loadimage 时缩放至以下尺寸）
#define PLAYER_STAND_W   72   // kazuma 站立宽
#define PLAYER_STAND_H  174   // kazuma 站立高
#define PLAYER_CROUCH_W  72   // kazuma 下蹲宽
#define PLAYER_CROUCH_H 114   // kazuma 下蹲高

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
static IMAGE img_player_run[6];  // 跑步动画帧 001~006
static IMAGE img_player2;   // kazuma 下蹲
static IMAGE img_obs1;      // lalatina 高障碍物
static IMAGE img_obs2;      // megume 矮宽障碍物
static IMAGE img_flying;    // aqura 飞行物
// 精灵遮罩图（用于透明渲染）
static IMAGE img_player_run_mask[6];
static IMAGE img_player2_mask;
static IMAGE img_obs1_mask;
static IMAGE img_obs2_mask;
static IMAGE img_flying_mask;
static bool g_imagesLoaded = false;
static IMAGE img_sky_bg;         // 预渲染天空渐变
static bool g_bgInitialized = false;

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
static void InitBackground();
static void DrawSky();
static void DrawHills();

// ========================================
// 精灵遮罩生成 + 透明贴图
// ========================================
// 生成精灵遮罩（黑/白/低Alpha → 视为透明背景）
static void GenerateSpriteMask(IMAGE* src, IMAGE* mask) {
    int w = src->getwidth();
    int h = src->getheight();
    mask->Resize(w, h);
    DWORD* srcBuf = GetImageBuffer(src);
    DWORD* maskBuf = GetImageBuffer(mask);
    if (srcBuf && maskBuf) {
        int total = w * h;
        for (int i = 0; i < total; i++) {
            DWORD pixel = srcBuf[i];
            BYTE alpha = (pixel >> 24) & 0xFF;
            DWORD rgb = pixel & 0x00FFFFFF;
            // 透明条件：低Alpha、纯黑、纯白
            if (alpha < 128 || rgb == 0x000000 || rgb == 0x00FFFFFF) {
                maskBuf[i] = 0x00FFFFFF; // 白 → SRCAND 保留背景
            } else {
                maskBuf[i] = 0x00000000; // 黑 → SRCAND 挖洞
            }
        }
    }
}

// 透明贴图：遮罩 SRCAND 挖洞 + 原图 SRCPAINT 填充
static void DrawSpriteTransparent(IMAGE* img, IMAGE* mask, int x, int y) {
    putimage(x, y, mask, SRCAND);
    putimage(x, y, img, SRCPAINT);
}

// ========================================
// 图片加载（只加载一次）
// ========================================
static void LoadGameImages() {
    if (g_imagesLoaded) return;
    // 跑步动画 6 帧（文件不存在时回退到 kazuma.png）
    TCHAR path[64];
    for (int i = 0; i < 6; i++) {
        _stprintf_s(path, _T("public/runner/%03d.png"), i + 1);

        if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
            loadimage(&img_player_run[i], path, PLAYER_STAND_W, PLAYER_STAND_H, true);
        else
            loadimage(&img_player_run[i], _T("public/kazuma.png"), PLAYER_STAND_W, PLAYER_STAND_H, true);
        GenerateSpriteMask(&img_player_run[i], &img_player_run_mask[i]);
    }
    loadimage(&img_player2, _T("public/kazuma_down.png"), PLAYER_CROUCH_W, PLAYER_CROUCH_H, true);
    GenerateSpriteMask(&img_player2, &img_player2_mask);
    loadimage(&img_obs1,    _T("public/lalatina.png"), 58,              144,              true);
    GenerateSpriteMask(&img_obs1, &img_obs1_mask);
    loadimage(&img_obs2,    _T("public/megume.png"),   120,             70,               true);
    GenerateSpriteMask(&img_obs2, &img_obs2_mask);
    loadimage(&img_flying,  _T("public/aqura.png"),    120,             72,               true);
    GenerateSpriteMask(&img_flying, &img_flying_mask);
    g_imagesLoaded = true;
}

// ========================================
// 预渲染背景资源（只运行一次）
// ========================================
static void InitBackground() {
    if (g_bgInitialized) return;

    // ---- 天空渐变 (1200×350) ----
    img_sky_bg.Resize(WINDOW_WID, GROUND_Y);
    SetWorkingImage(&img_sky_bg);
    for (int y = 0; y < GROUND_Y; y++) {
        float t = (float)y / (GROUND_Y - 1);
        int r = (int)(80  + (180 - 80)  * t);
        int g = (int)(150 + (220 - 150) * t);
        int b = (int)(240 + (255 - 240) * t);
        setfillcolor(RGB(r, g, b));
        solidrectangle(0, y, WINDOW_WID, y + 1);
    }
    SetWorkingImage(NULL);
    g_bgInitialized = true;
}

// ========================================
// 绘制天空（渐变 + 飘动云朵）
// ========================================
static void DrawSky() {
    // 天空渐变
    putimage(0, 0, &img_sky_bg);

    // 云朵定义 (相对X, Y, 半径X, 半径Y)
    static const struct { int relX, y, rx, ry; } clouds[] = {
        { 100,  50,  70, 24},
        { 350,  70,  85, 28},
        { 580,  45,  55, 18},
        { 780,  85,  75, 22},
        { 950,  60,  50, 16},
        { 200, 120,  65, 20},
        { 850, 100,  60, 20},
    };
    const int CLOUD_COUNT = sizeof(clouds) / sizeof(clouds[0]);

    int cloudOffset = (int)(gs.frameCount * 0.3f) % WINDOW_WID;

    setfillcolor(CLOUD_COLOR);
    for (int i = 0; i < CLOUD_COUNT; i++) {
        int cx = (clouds[i].relX - cloudOffset + WINDOW_WID) % WINDOW_WID;
        int cy = clouds[i].y;
        int rx = clouds[i].rx;
        int ry = clouds[i].ry;

        // 主体椭圆 + 左右 puff
        solidellipse(cx - rx,       cy - ry, cx + rx,       cy + ry);
        solidellipse(cx - rx * 2/3, cy,      cx + rx / 3,   cy + ry * 3/4);
        solidellipse(cx - rx / 3,   cy,      cx + rx * 2/3, cy + ry * 3/4);

        // 边缘包装：如果云朵超出屏幕边界，在对侧补画
        if (cx + rx > WINDOW_WID) {
            int cx2 = cx - WINDOW_WID;
            solidellipse(cx2 - rx,       cy - ry, cx2 + rx,       cy + ry);
            solidellipse(cx2 - rx * 2/3, cy,      cx2 + rx / 3,   cy + ry * 3/4);
            solidellipse(cx2 - rx / 3,   cy,      cx2 + rx * 2/3, cy + ry * 3/4);
        }
        if (cx - rx < 0) {
            int cx2 = cx + WINDOW_WID;
            solidellipse(cx2 - rx,       cy - ry, cx2 + rx,       cy + ry);
            solidellipse(cx2 - rx * 2/3, cy,      cx2 + rx / 3,   cy + ry * 3/4);
            solidellipse(cx2 - rx / 3,   cy,      cx2 + rx * 2/3, cy + ry * 3/4);
        }
    }
}

// ========================================
// 绘制山丘（双层正弦波视差）
// ========================================
static void DrawHills() {
    const int POINTS = 80;
    const float M_2PI = 6.283185307f;
    int pts[(POINTS + 2) * 2];  // EasyX fillpoly 使用平铺 int 数组 (x0,y0, x1,y1, ...)

    // ---- 远山 (深色, 慢) ----
    float farScroll = (float)((int)(gs.frameCount * gs.speed * HILL_FAR_SPEED) % HILL_TILE_W);
    setfillcolor(HILL_FAR_COLOR);

    for (int i = 0; i < POINTS; i++) {
        float t = (float)i / (POINTS - 1);
        int x = (int)(-50 + t * (WINDOW_WID + 100));
        int y = HILL_BASE_Y - (int)(HILL_FAR_AMPL * sinf(M_2PI * 2.5f * (x + farScroll) / HILL_TILE_W));
        pts[i * 2]     = x;
        pts[i * 2 + 1] = y;
    }
    pts[POINTS * 2]         = WINDOW_WID + 50;
    pts[POINTS * 2 + 1]     = GROUND_Y;
    pts[(POINTS + 1) * 2]     = -50;
    pts[(POINTS + 1) * 2 + 1] = GROUND_Y;
    fillpoly(POINTS + 2, pts);

    // ---- 近山 (浅色, 快) ----
    float nearScroll = (float)((int)(gs.frameCount * gs.speed * HILL_NEAR_SPEED) % HILL_TILE_W);
    setfillcolor(HILL_NEAR_COLOR);

    for (int i = 0; i < POINTS; i++) {
        float t = (float)i / (POINTS - 1);
        int x = (int)(-50 + t * (WINDOW_WID + 100));
        int y = HILL_BASE_Y - (int)(HILL_NEAR_AMPL * sinf(M_2PI * 4.0f * (x + nearScroll) / HILL_TILE_W));
        pts[i * 2]     = x;
        pts[i * 2 + 1] = y;
    }
    pts[POINTS * 2]         = WINDOW_WID + 50;
    pts[POINTS * 2 + 1]     = GROUND_Y;
    pts[(POINTS + 1) * 2]     = -50;
    pts[(POINTS + 1) * 2 + 1] = GROUND_Y;
    fillpoly(POINTS + 2, pts);
}

// ========================================
// 初始化 / 重置游戏状态
// ========================================
static void InitGameState() {
    srand((unsigned)time(NULL));

    // 玩家
    gs.player.x = PLAYER_X;
    gs.player.y = GROUND_Y - PLAYER_STAND_H;
    gs.player.vy = 0;
    gs.player.w = PLAYER_STAND_W;
    gs.player.h = PLAYER_STAND_H;
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

    // ---- 按时间加分（每 1 秒 +1 分） ----
    if (gs.frameCount % 60 == 0) {
        gs.score++;
    }

    // ---- 随时间加速（每 15 秒 +1，上限 MAX_SPEED） ----
    int targetSpeed = BASE_SPEED + gs.frameCount / 900;
    if (targetSpeed > MAX_SPEED) targetSpeed = MAX_SPEED;
    gs.speed = targetSpeed;

    // ---- 跳跃物理 ----
    if (p.jumping) {
        p.y += (int)p.vy;
        p.vy += GRAVITY;
        // 落地检测（始终以站立高度为准）
        int groundLevel = GROUND_Y - PLAYER_STAND_H;
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
            p.y = GROUND_Y - PLAYER_STAND_H;
            p.h = PLAYER_STAND_H;
            p.w = PLAYER_STAND_W;
        }
    }

    // ---- 移动障碍物 ----
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        Obstacle& o = gs.obstacles[i];
        if (!o.active) continue;

        o.x -= gs.speed + (o.flying ? FLYING_SPEED_BONUS : 0);

        // 离开左边界
        if (o.x + o.w < 0) {
            o.active = false;
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
            // 写入排行榜（分数 + 存活秒数）
            double survivalTime = (double)(gs.frameCount * FRAME_DELAY) / 1000.0;
            tuple<int, double> record(gs.score, survivalTime);
            writeRank(record);
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
            p.animFrame = (p.animFrame + 1) % 6;
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

    DrawSky();          // 天空渐变 + 云朵
    DrawHills();        // 双层山丘视差滚动
    DrawGround();       // 草地 + 地面
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
    if (p.crouching) {
        DrawSpriteTransparent(&img_player2, &img_player2_mask, p.x, p.y);
    } else {
        DrawSpriteTransparent(&img_player_run[p.animFrame], &img_player_run_mask[p.animFrame], p.x, p.y);
    }
}

// ========================================
// 绘制障碍物
// ========================================
static void DrawObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        const Obstacle& o = gs.obstacles[i];
        if (!o.active) continue;

        if (o.flying) {
            DrawSpriteTransparent(&img_flying, &img_flying_mask, o.x, o.y);
        } else {
            IMAGE* img  = (o.type == 0) ? &img_obs1 : &img_obs2;
            IMAGE* mask = (o.type == 0) ? &img_obs1_mask : &img_obs2_mask;
            DrawSpriteTransparent(img, mask, o.x, GROUND_Y - o.drawH);
        }
    }
}

// ========================================
// 绘制地面 + 滚动虚线
// ========================================
static void DrawGround() {
    // 草地
    setfillcolor(RGB(55, 150, 40));
    solidrectangle(0, GROUND_Y, WINDOW_WID, GROUND_Y + GRASS_STRIP_H);

    // 草地边缘
    setcolor(RGB(30, 90, 20));
    setlinestyle(PS_SOLID, 2);
    line(0, GROUND_Y, WINDOW_WID, GROUND_Y);

    // 泥土
    setfillcolor(RGB(139, 90, 43));
    solidrectangle(0, GROUND_Y + GRASS_STRIP_H, WINDOW_WID, WINDOW_HEI);

    // 滚动虚线
    int offset = (gs.frameCount * (gs.speed - 2)) % 40;
    if (offset < 0) offset += 40;

    setcolor(RGB(160, 120, 60));
    setlinestyle(PS_SOLID, 2);
    for (int x = -offset; x < WINDOW_WID; x += 40) {
        line(x,     GROUND_Y + GRASS_STRIP_H + 6,  x + 18, GROUND_Y + GRASS_STRIP_H + 6);
        line(x + 12, GROUND_Y + GRASS_STRIP_H + 22, x + 30, GROUND_Y + GRASS_STRIP_H + 22);
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
    // 居中半透明面板
    int panelW = 420;
    int panelH = 110;
    int panelX = (WINDOW_WID - panelW) / 2;
    int panelY = (WINDOW_HEI - panelH) / 2 - 30;

    // 面板背景（深色圆角效果用多层矩形模拟）
    setfillcolor(RGB(0, 0, 0));
    solidrectangle(panelX, panelY, panelX + panelW, panelY + panelH);

    // 面板边框
    setcolor(RGB(80, 80, 80));
    setlinestyle(PS_SOLID, 2);
    rectangle(panelX, panelY, panelX + panelW, panelY + panelH);

    settextstyle(36, 0, _T("微软雅黑"));
    setbkmode(TRANSPARENT);
    setcolor(RGB(255, 255, 255));

    RECT r = { panelX, panelY + 8, panelX + panelW, panelY + 48 };
    drawtext(_T("暂停中"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    settextstyle(18, 0, _T("微软雅黑"));
    setcolor(RGB(200, 200, 200));

    r = { panelX, panelY + 55, panelX + panelW, panelY + 95 };
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
    InitBackground();       // 预渲染天空渐变
    InitGameState();
    g_inGame = true;
    BeginBatchDraw();          // 开启双缓冲，消除闪烁
    play_music(TEXT("public/gaming.mp3"));

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

    stop_music();
    EndBatchDraw();            // 退出游戏循环，关闭双缓冲
    g_inGame = false;
    return 1;   // 返回菜单（真正的退出在菜单中选"退出游戏"）
}
