#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 800
#define HEIGHT 300
#define GROUND_Y 250
#define DINO_X 100

// 你指定的参数
#define JUMP_VELOCITY -12
#define GRAVITY 1
#define OBSTACLE_FIX_H 45

// 翼龙配置
#define BIRD_Y 180
#define BIRD_W 40
#define BIRD_H 20

// 恐龙结构体
typedef struct {
    int y;
    int vy;
    int isJump;
    int isDown;
} Dino;

// 仙人掌障碍物
typedef struct {
    int x, w, h;
} Obstacle;

// 翼龙结构体
typedef struct {
    int x;
    int isExist;
} Bird;

Dino dino;
Obstacle obs;
Bird bird;
int score = 0;
int speed = 6;
int isGameOver = 0;
int cloudX = 600;

// 初始化游戏
void initGame() {
    dino.y = GROUND_Y - 40;
    dino.vy = 0;
    dino.isJump = 0;
    dino.isDown = 0;

    obs.x = WIDTH + 100;
    obs.w = 25;
    obs.h = OBSTACLE_FIX_H;

    bird.x = WIDTH + 500;
    bird.isExist = 0;

    score = 0;
    speed = 6;
    isGameOver = 0;
    srand((unsigned)time(NULL));
}

// 绘制恐龙
void drawDino() {
    setfillcolor(RGB(80, 80, 80));
    if (dino.isDown) {
        solidrectangle(DINO_X, dino.y + 20, DINO_X + 40, dino.y + 40);
    }
    else {
        solidrectangle(DINO_X, dino.y, DINO_X + 30, dino.y + 40);
        solidrectangle(DINO_X + 5, dino.y - 15, DINO_X + 25, dino.y);
    }
}

// 绘制仙人掌
void drawObstacle() {
    setfillcolor(RGB(30, 120, 30));
    solidrectangle(obs.x, GROUND_Y - obs.h, obs.x + obs.w, GROUND_Y);
    solidrectangle(obs.x - 8, GROUND_Y - obs.h + 10, obs.x, GROUND_Y - obs.h + 20);
    solidrectangle(obs.x + obs.w, GROUND_Y - obs.h + 20, obs.x + obs.w + 8, GROUND_Y - obs.h + 30);
}

// 绘制翼龙
void drawBird() {
    if (bird.isExist) {
        setfillcolor(RGB(60, 60, 60));
        solidrectangle(bird.x, BIRD_Y, bird.x + BIRD_W, BIRD_Y + BIRD_H);
        solidrectangle(bird.x - 10, BIRD_Y - 5, bird.x, BIRD_Y + 5);
        solidrectangle(bird.x + BIRD_W, BIRD_Y - 5, bird.x + BIRD_W + 10, BIRD_Y + 5);
    }
}

// 绘制地面
void drawGround() {
    setcolor(RGB(100, 100, 100));
    line(0, GROUND_Y, WIDTH, GROUND_Y);
    setcolor(RGB(180, 180, 180));
    for (int i = 0; i < WIDTH; i += 40) {
        line(i, GROUND_Y + 5, i + 20, GROUND_Y + 5);
    }
}

// 绘制云朵
void drawCloud() {
    setfillcolor(RGB(220, 220, 220));
    solidcircle(cloudX, 60, 20);
    solidcircle(cloudX + 25, 60, 25);
    solidcircle(cloudX + 50, 60, 20);
    solidrectangle(cloudX, 60 - 15, cloudX + 50, 60 + 15);
}

// 绘制分数
void drawScore() {
    setcolor(RGB(50, 50, 50));
    settextstyle(24, 0, _T("Consolas"));
    TCHAR str[30];
    _stprintf_s(str, _T("Score: %d"), score);
    outtextxy(10, 10, str);
}

// 游戏结束界面
void drawGameOver() {
    setcolor(RED);
    settextstyle(40, 0, _T("Consolas"));
    outtextxy(WIDTH / 2 - 140, HEIGHT / 2 - 30, _T("GAME OVER"));
    settextstyle(24, 0, _T("Consolas"));
    outtextxy(WIDTH / 2 - 200, HEIGHT / 2 + 20, _T("Press R to Restart"));
}

// 跳跃（会自动取消下蹲）
void jump() {
    if (!dino.isJump) {
        dino.isDown = 0;       // 跳跃时自动站起来
        dino.y = GROUND_Y - 40;
        dino.isJump = 1;
        dino.vy = JUMP_VELOCITY;
    }
}

// 切换下蹲（按一下就保持）
void toggleDown() {
    if (!dino.isJump) {
        dino.isDown = !dino.isDown;
        if (dino.isDown) {
            dino.y = GROUND_Y - 20;
        }
        else {
            dino.y = GROUND_Y - 40;
        }
    }
}

// 碰撞检测
int checkCollision() {
    int dL = DINO_X + 5;
    int dR = DINO_X + 25;
    int dT = dino.y + 5;
    int dB = dino.y + 40;

    // 仙人掌碰撞
    int oL = obs.x;
    int oR = obs.x + obs.w;
    int oT = GROUND_Y - obs.h;
    int oB = GROUND_Y;
    if (dR > oL && dL < oR && dB > oT && dT < oB)
        return 1;

    // 翼龙碰撞
    if (bird.isExist && !dino.isDown) {
        int bL = bird.x;
        int bR = bird.x + BIRD_W;
        int bT = BIRD_Y;
        int bB = BIRD_Y + BIRD_H;
        if (dR > bL && dL < bR && dB > bT && dT < bB)
            return 1;
    }
    return 0;
}

int main() {
    initgraph(WIDTH, HEIGHT);
    setbkcolor(WHITE);
    initGame();
    BeginBatchDraw();

    while (1) {
        cleardevice();

        // 按键监听（一次触发，不按住）
        if (_kbhit()) {
            char key = (char)_getch();
            if (key == ' ' || key == 72) jump();          // 空格/上键 跳跃 + 站起
            if (key == 80) toggleDown();                  // 下键 切换下蹲/站起
            if (key == -32) continue;
            if ((key == 'r' || key == 'R') && isGameOver) // R 重启
                initGame();
        }

        if (isGameOver) {
            drawGameOver();
            FlushBatchDraw();
            Sleep(16);
            continue;
        }

        // 跳跃物理
        if (dino.isJump) {
            dino.y += dino.vy;
            dino.vy += GRAVITY;
            if (dino.y >= GROUND_Y - 40) {
                dino.y = GROUND_Y - 40;
                dino.isJump = 0;
                dino.vy = 0;
            }
        }

        // 仙人掌移动
        obs.x -= speed;
        if (obs.x < -obs.w) {
            obs.x = WIDTH + rand() % 200;
            score++;
            if (score % 15 == 0 && speed < 12)
                speed++;
        }

        // 翼龙高频出现
        if (score >= 5 && !bird.isExist && rand() % 40 == 0) {
            bird.isExist = 1;
            bird.x = WIDTH;
        }
        if (bird.isExist) {
            bird.x -= speed + 2;
            if (bird.x < -BIRD_W) {
                bird.isExist = 0;
            }
        }

        // 云朵
        cloudX -= 1;
        if (cloudX < -60)
            cloudX = WIDTH + rand() % 200;

        // 碰撞判断
        if (checkCollision())
            isGameOver = 1;

        // 绘制
        drawCloud();
        drawGround();
        drawDino();
        drawObstacle();
        drawBird();
        drawScore();

        FlushBatchDraw();
        Sleep(16);
    }

    closegraph();
    return 0;
}