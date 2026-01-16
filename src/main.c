#define SDL_MAIN_USE_CALLBACKS 1
#include "batchingRender.h"
#include "camera.h"
#include "character.h"
#include "frameController.h"
#include "polygon.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 屏幕尺寸
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define LOGICAL_WIDTH 1920
#define LOGICAL_HEIGHT 1080

// 游戏逻辑帧率
#define LOGIC_FRAME_RATE 60

// 场景枚举
typedef enum
{
    SCENE_MAIN_MENU,
    SCENE_GAME_PLAY,
    SCENE_GAME_OVER,
    SCENE_EXIT
} GameScene;

// 场景函数指针类型
typedef void (*SceneInitFunc)(void);
typedef void (*SceneCleanupFunc)(void);
typedef SDL_AppResult (*SceneEventFunc)(SDL_Event *event);
typedef SDL_AppResult (*SceneUpdateFunc)(void);
typedef void (*SceneRenderFunc)(void);

// 场景结构体
typedef struct
{
    GameScene id; // 同时也是名字
    SceneInitFunc init;
    SceneCleanupFunc cleanup;
    SceneEventFunc handle_event;
    SceneUpdateFunc update;
    SceneRenderFunc render;
} Scene;

// 场景函数声明
void MainMenuScene_Init(void);
void MainMenuScene_Cleanup(void);
SDL_AppResult MainMenuScene_Event(SDL_Event *event);
SDL_AppResult MainMenuScene_Update(void);
void MainMenuScene_Render(void);

void GamePlayScene_Init(void);
void GamePlayScene_Cleanup(void);
SDL_AppResult GamePlayScene_Event(SDL_Event *event);
SDL_AppResult GamePlayScene_Update(void);
void GamePlayScene_Render(void);

void GameOverScene_Init(void);
void GameOverScene_Cleanup(void);
SDL_AppResult GameOverScene_Event(SDL_Event *event);
SDL_AppResult GameOverScene_Update(void);
void GameOverScene_Render(void);

// 全局变量
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static BatchRenderer *g_worldBatch = NULL; // 世界几何批次
static UIManager *g_uiManager = NULL;      // UI管理器
static TTF_Font *g_font = NULL;
static Camera *g_camera = NULL;
float FPS = 0.0f;
float UPS = 0.0f;

int score = 0;
int maxScore = 0;
float playSceneTime = 0.0f;
Uint64 start = 0;
Uint64 now = 0;
bool select = false;

float lastSpawnTime = 0.0f;
int spawnInterval = 5; // 初始刷怪间隔为5秒

enum BulletType selectedBulletType = AMMO;
char selectedBulletName[3][20] = {"Ammo", "Bubblle", "sniperBullet"};
enum GunType selectedGunType = SHORTGUN;
Bullet selectedBullet = {};
Gun selectedGun = {};

// ---------------全局场景状态--------------------
// 当前场景状态
static GameScene g_currentScene = SCENE_MAIN_MENU;
static GameScene g_nextScene = SCENE_MAIN_MENU;
static bool g_sceneChanged = true; // 初始化为true，以便首次初始化

// 场景数组
static Scene g_scenes[] = {
    {SCENE_MAIN_MENU, MainMenuScene_Init, MainMenuScene_Cleanup, MainMenuScene_Event, MainMenuScene_Update, MainMenuScene_Render},

    {SCENE_GAME_PLAY, GamePlayScene_Init, GamePlayScene_Cleanup, GamePlayScene_Event, GamePlayScene_Update, GamePlayScene_Render},

    {SCENE_GAME_OVER, GameOverScene_Init, GameOverScene_Cleanup, GameOverScene_Event, GameOverScene_Update, GameOverScene_Render},
};

// 场景切换函数
void ChangeScene(GameScene newScene)
{
    if (newScene != g_currentScene)
    {
        g_nextScene = newScene;
        g_sceneChanged = true;
        SDL_Log("Requesting scene change to: %d", newScene);
    }
}

// ==================== 场景共享资源 ====================

// 以下资源在不同场景间共享，但使用时要注意场景切换时的状态
static CharacterPool g_characterPool;
static BulletPool g_bulletPool;
static Character *g_playerCharacter = NULL; // 玩家角色
static FrameController g_frameController;

// 游戏控制状态（游戏场景专用）
static struct
{
    bool cameraMoveUP;
    bool cameraMoveDown;
    bool cameraMoveLeft;
    bool cameraMoveRight;
    bool moveForward;
    bool turnLeft;
    bool turnRight;
    bool headShoot;
    bool tailShoot;
} g_gameControls;

// 图片纹理
SDL_Texture *shortGunTexture;
SDL_Texture *longGunTexture;
SDL_Texture *sniperGunTexture;
SDL_Texture *textures[3];

// 角色预设
// 蛇1:
const int SNAKE1_bodyCount = 16;
const float SNAKE1_radiusList[16] = {30, 30, 25, 25, 25, 25, 25, 25, 25, 20, 20, 15, 15, 15, 10, 10};
const float SNAKE1_distanceList[16] = {60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60};
const float SNAKE1_flexibility[16] = {45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f};
// 蜥蜴
const int LIZARD_bodyCount = 21;
const float LIZARD_radiusList[21] = {55, 31, 58, 59, 60, 60, 48, 29, 18, 11, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7};
const float LIZARD_distanceList[21] = {42, 37, 29, 30, 33, 33, 28, 18, 14, 14, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
const float LIZARD_flexibility[21] = {45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f};
const Chain3 LIZARD_legs[2] = {{{0, 0}, {0.0}, {0.0}, 40.0f, 20.0f, 20.0f, 70.0f, 35.0f}, {{0, 0}, {0.0}, {0.0}, 23.0f, 23.0f, 23.0f, 80.0f, 45.0f}};

// 颜色预设
const SDL_FColor darkGold = {0.85f, 0.64f, 0.13f, 1.0f};
const SDL_FColor lightSkyBlue = {5.29f, 0.81f, 0.98f, 1.0f};
const SDL_FColor purple = {0.58f, 0.0f, 0.83f, 1.0f};
const SDL_FColor white = {1.0f, 1.0f, 1.0f, 1.0f};

// 子弹预设
const Bullet defalutBullet = {5, 5, 5, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, 0, (Vector){0, 0}};
const Bullet ammo = {10, 20, 10, darkGold, 0, (Vector){0, 0}};
const Bullet bubble = {100, 5, 25, lightSkyBlue, 0, (Vector){0, 0}};
const Bullet sniperBullet = {75, 40, 15, purple, 0, (Vector){0, 0}};

// 枪预设
const Gun shortGun = {SHORTGUN, defalutBullet, 90, 1.25, 0};
const Gun longGun = {LONGGUN, defalutBullet, 20, 1.0, 0};
const Gun sniperGun = {SNIPERGUN, defalutBullet, 150, 2.0, 0};

// ui矩行
const SDL_FRect titleRect = {SCREEN_WIDTH / 2.0f - 200, 100, 400, 80};
const SDL_FRect startRect = {SCREEN_WIDTH / 2.0f - 100, 450, 200, 60};
const SDL_FRect exitRect = {SCREEN_WIDTH / 2.0f - 100, 650, 200, 60};
const SDL_FRect gameoverTitleRect = {SCREEN_WIDTH / 2.0f - 200, 100, 400, 80};
const SDL_FRect restartRect = {SCREEN_WIDTH / 2.0f - 100, 450, 200, 60};
const SDL_FRect toMeneRect = {SCREEN_WIDTH / 2.0f - 100, 650, 200, 60};

// 文本矩行
const SDL_FRect scoreTextRect = {SCREEN_WIDTH / 2.0f - 125, 10, 250, 40};
const SDL_FRect timeTextRect = {SCREEN_WIDTH / 2.0f - 100, SCREEN_HEIGHT - 60, 200, 60};
// 墙BOX
//-SCREEN_WIDTH*3,SCREEN_WIDTH*3,-SCREEN_HEIGHT*3,SCREEN_HEIGHT*3
const AABBBox wallUpBox = {-SCREEN_WIDTH * 4, SCREEN_WIDTH * 4, SCREEN_HEIGHT * 3, SCREEN_HEIGHT * 4};
const AABBBox wallDownBox = {-SCREEN_WIDTH * 4, SCREEN_WIDTH * 4, -SCREEN_HEIGHT * 4, -SCREEN_HEIGHT * 3};
const AABBBox wallLeftBox = {-SCREEN_WIDTH * 4, -SCREEN_WIDTH * 3, -SCREEN_HEIGHT * 4, SCREEN_HEIGHT * 4};
const AABBBox wallRightBox = {SCREEN_WIDTH * 3, SCREEN_WIDTH * 4, -SCREEN_HEIGHT * 4, SCREEN_HEIGHT * 4};
SDL_FRect wallUpRect;
SDL_FRect wallDownRect;
SDL_FRect wallLeftRect;
SDL_FRect wallRightRect;

// ==================== 辅助函数 ====================

void CreatePlayerCharacter(void)
{
    if (!g_worldBatch) return;

    // 创建蜥蜴角色作为玩家
    g_playerCharacter = Character_Creat(LIZARD, 100.f, 100.f, (Vector){-1.0f, -0.5f}, 5, LIZARD_radiusList, LIZARD_distanceList, LIZARD_flexibility, LIZARD_bodyCount, (SDL_FColor){1.0f, 0.5f, 0.0f, 1.0f}, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, LIZARD_legs);

    if (g_playerCharacter)
    {
        CharacterPool_Add(&g_characterPool, g_playerCharacter);

        // 设置武器
        g_playerCharacter->haveHeadGun = true;
        g_playerCharacter->haveTailGun = true;
        g_playerCharacter->headGun = shortGun;
        g_playerCharacter->tailGun = shortGun;
        g_playerCharacter->headGun.bullet = defalutBullet;
        g_playerCharacter->tailGun.bullet = defalutBullet;

        SDL_Log("Player character created");
    }
}

void generateEnemyCharacter(void)
{
    // 获取玩家位置
    float playerX = g_playerCharacter->body[0].x;
    float playerY = g_playerCharacter->body[0].y;

    // 获取相机视野范围
    SDL_FRect viewRect = Camera_GetViewRect(g_camera);
    float viewWidth = viewRect.w;
    float viewHeight = viewRect.h;

    // 在玩家周围一定距离外生成敌人，确保它们在屏幕边缘外
    float spawnDistance = SDL_max(viewWidth, viewHeight) * 0.6f; // 生成距离为视野对角线长度的60%

    // 随机角度
    float angle = (float)(rand() % 360) * 3.14159f / 180.0f;

    // 根据角度计算生成位置
    float spawnX = playerX + spawnDistance * cosf(angle);
    float spawnY = playerY + spawnDistance * sinf(angle);

    // 确保生成位置在地图范围内
    spawnX = SDL_clamp(spawnX, -SCREEN_WIDTH * 3.0f, SCREEN_WIDTH * 3.0f);
    spawnY = SDL_clamp(spawnY, -SCREEN_HEIGHT * 3.0f, SCREEN_HEIGHT * 3.0f);

    // 生成朝向玩家的方向向量
    Vector dirToPlayer = {playerX - spawnX, playerY - spawnY};
    vector_Normalization(&dirToPlayer);

    // 随机颜色
    SDL_FColor randColor = {(float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, 1.0f};
    SDL_FColor randOutLineColor = {(float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, 1.0f};

    // 创建敌人
    Character *enemyCharacter = Character_Creat(SNAKE, spawnX, spawnY, dirToPlayer, 3.0f + fmodf(rand(), 3.0f), SNAKE1_radiusList, SNAKE1_distanceList, SNAKE1_flexibility, SNAKE1_bodyCount, randColor, randOutLineColor, NULL); // 随机速度 3.0~6.0

    if (enemyCharacter)
    {
        CharacterPool_Add(&g_characterPool, enemyCharacter);
    }
}

void CreateTestCharacters(void) // for debug&&性能测试
{
    g_playerCharacter->maxHP = 9999999;
    g_playerCharacter->HP = g_playerCharacter->maxHP;
    srand(time(NULL));
    for (int i = 0; i < 500; i++)
    { // 减少数量以提升性能
        SDL_FColor randColor = {(float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, 1.0f};
        SDL_FColor randOutLineColor = {(float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, (float)(rand() % 255) / 255.0f, 1.0f};
        Vector randDir = {(float)(rand() % 1000 - 500), (float)(rand() % 1000 - 500)};
        Character *tempCharacter;
        if (i % 2 == 0)
        {

            tempCharacter = Character_Creat(SNAKE, rand() % 5000, rand() % 500, randDir, 5, SNAKE1_radiusList, SNAKE1_distanceList, SNAKE1_flexibility, SNAKE1_bodyCount, randColor, randOutLineColor, NULL);
        }
        else
        {
            tempCharacter = Character_Creat(LIZARD, rand() % 5000, rand() % 500, randDir, 5, LIZARD_radiusList, LIZARD_distanceList, LIZARD_flexibility, LIZARD_bodyCount, randColor, randOutLineColor, LIZARD_legs);
        }
        if (tempCharacter)
        {
            CharacterPool_Add(&g_characterPool, tempCharacter);
        }
    }
}

// 渲染网格背景
static void RenderGridBackground(Camera *camera, SDL_Renderer *renderer)
{
    if (!camera || !renderer) return;

    // 获取屏幕中心对应的世界坐标
    float centerWorldX = camera->x;
    float centerWorldY = camera->y;

    // 计算从屏幕中心到屏幕边缘的世界坐标距离
    float maxWorldWidth = (float)camera->screenWidth / camera->zoom;
    float maxWorldHeight = (float)camera->screenHeight / camera->zoom;

    // 计算需要绘制的网格范围（世界坐标）
    float leftWorld = centerWorldX - maxWorldWidth * 0.5f;
    float rightWorld = centerWorldX + maxWorldWidth * 0.5f;
    float bottomWorld = centerWorldY - maxWorldHeight * 0.5f;
    float topWorld = centerWorldY + maxWorldHeight * 0.5f;

    // 对齐到网格
    int gridSize = 50;
    int startX = (int)SDL_floorf(leftWorld / gridSize) * gridSize;
    int endX = (int)SDL_ceilf(rightWorld / gridSize) * gridSize;
    int startY = (int)SDL_floorf(bottomWorld / gridSize) * gridSize;
    int endY = (int)SDL_ceilf(topWorld / gridSize) * gridSize;

    // 设置绘制颜色
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);

    // 绘制垂直线
    for (int x = startX; x <= endX; x += gridSize)
    {
        SDL_FPoint p1 = Camera_WorldToScreen(camera, (float)x, (float)startY);
        SDL_FPoint p2 = Camera_WorldToScreen(camera, (float)x, (float)endY);
        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    }

    // 绘制水平线
    for (int y = startY; y <= endY; y += gridSize)
    {
        SDL_FPoint p1 = Camera_WorldToScreen(camera, (float)startX, (float)y);
        SDL_FPoint p2 = Camera_WorldToScreen(camera, (float)endX, (float)y);
        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    }
}

// 渲染FPS/UPS
static void RenderFPSDisplay(float fps, float ups)
{
    if (!g_font || !g_renderer) return;
    char fpsText[64];
    snprintf(fpsText, sizeof(fpsText), "FPS: %.1f|UPS:%.1f", fps, ups);
    Draw_Text(g_font, g_renderer, (SDL_FRect){10, 5, 250, 30}, (SDL_FColor){0.0f, 1.0f, 0.0f, 1.0f}, fpsText);
}

// ==================== 主菜单场景实现 ====================

void MainMenuScene_Init(void)
{
    SDL_Log("Initializing Main Menu scene");

    // 重置UI管理器（如果已存在则重新创建）
    if (g_uiManager)
    {
        UI_DestroyManager(g_uiManager);
    }
    g_uiManager = UI_CreateManager();

    // 重置相机（主菜单使用固定相机）
    if (g_camera)
    {
        Camera_Destroy(g_camera);
    }
    g_camera = Camera_Create(0.0f, 0.0f, LOGICAL_WIDTH, LOGICAL_HEIGHT);

    // 重置批次渲染器
    if (g_worldBatch)
    {
        Batch_DestroyRenderer(g_worldBatch);
    }
    g_worldBatch = Batch_CreateRenderer(2048, 6144);

    // 重置帧控制器
    FrameController_Init(&g_frameController, LOGIC_FRAME_RATE);
}

void MainMenuScene_Cleanup(void) { SDL_Log("Cleaning up Main Menu scene"); }

SDL_AppResult MainMenuScene_Event(SDL_Event *event)
{

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        ChangeScene(SCENE_EXIT);
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE)
        {
            ChangeScene(SCENE_EXIT);
            return SDL_APP_SUCCESS;
        }

        // 按回车或空格开始游戏
        if (event->key.key == SDLK_RETURN || event->key.key == SDLK_SPACE)
        {
            ChangeScene(SCENE_GAME_PLAY);
            return SDL_APP_CONTINUE;
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // 点击开始游戏
        if (PointInRect(event->button.x, event->button.y, startRect))
        {
            ChangeScene(SCENE_GAME_PLAY);
        }
        // 点击退出游戏
        if (PointInRect(event->button.x, event->button.y, exitRect))
        {
            ChangeScene(SCENE_EXIT);
            return SDL_APP_SUCCESS;
        }
        break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult MainMenuScene_Update(void)
{
    // 主菜单只需要更新帧控制器以计算FPS
    FrameController_Update(&g_frameController);
    return SDL_APP_CONTINUE;
}

void MainMenuScene_Render(void)
{
    // 清除屏幕
    SDL_SetRenderDrawColor(g_renderer, 30, 30, 50, 255);
    SDL_RenderClear(g_renderer);

    // 渲染菜单背景
    if (g_worldBatch)
    {
        Batch_Clear(g_worldBatch);

        // 绘制背景方块
        for (int i = 0; i < 20; i++)
        {
            float x = (i % 5) * 150 - 300;
            float y = (i / 5.0f) * 150 - 300;
            SDL_FColor color = {0.1f + i * 0.02f, 0.2f, 0.3f + i * 0.01f, 0.7f};
            Polygon_DrawRect(g_worldBatch, x, y, 140, 140, color, g_camera);
        }

        Batch_Render(g_worldBatch, g_renderer);
    }

    // 渲染UI（菜单选项）
    if (g_uiManager)
    {
        UI_BeginDraw(g_uiManager);

        // 标题
        UI_DrawRect(g_uiManager, titleRect.x, titleRect.y, titleRect.w, titleRect.h, (SDL_FColor){0.2f, 0.4f, 0.6f, 0.9f}, true);

        // 开始按钮
        UI_DrawRect(g_uiManager, startRect.x, startRect.y, startRect.w, startRect.h, (SDL_FColor){0.3f, 0.7f, 0.3f, 0.9f}, true);

        // 退出按钮
        UI_DrawRect(g_uiManager, exitRect.x, exitRect.y, exitRect.w, exitRect.h, (SDL_FColor){0.7f, 0.3f, 0.3f, 0.9f}, true);

        UI_EndDraw(g_uiManager, g_renderer);
    }

    // 渲染文字
    if (g_font)
    {

        // 标题文字
        Draw_Text(g_font, g_renderer, titleRect, white, "LIZARD, STAY ALIVE!");

        // 开始按钮文字
        Draw_Text(g_font, g_renderer, startRect, white, "START GAME");

        // 退出按钮文字
        Draw_Text(g_font, g_renderer, exitRect, white, "EXIT GAME");
    }

    // 渲染测试纹理
    static float testangle = 0;
    testangle = fmodf(testangle + 0.1, 360.0f);
    Render_Texture(g_renderer, shortGunTexture, (SDL_FPoint){100, 100}, testangle, 10);
    Render_Texture(g_renderer, longGunTexture, (SDL_FPoint){SCREEN_WIDTH - 100, 100}, testangle, 10);
    Render_Texture(g_renderer, sniperGunTexture, (SDL_FPoint){100, SCREEN_HEIGHT - 100}, testangle, 10);

    // 渲染FPS
    FrameController_UpdateFPS(&g_frameController, &FPS, &UPS);
    RenderFPSDisplay(FPS, UPS);

    SDL_RenderPresent(g_renderer);
    FrameController_AddRenderCount(&g_frameController);
}

// ==================== 游戏场景实现 ====================

void GamePlayScene_Init(void)
{
    SDL_Log("Initializing Game Play scene");

    // 重置UI管理器
    if (g_uiManager)
    {
        UI_DestroyManager(g_uiManager);
    }
    g_uiManager = UI_CreateManager();

    // 重置相机（游戏场景使用跟随相机）
    if (g_camera)
    {
        Camera_Destroy(g_camera);
    }
    g_camera = Camera_Create(0.0f, 0.0f, LOGICAL_WIDTH, LOGICAL_HEIGHT);
    Camera_SetBounds(g_camera, -SCREEN_WIDTH * 3, SCREEN_WIDTH * 3, -SCREEN_HEIGHT * 3, SCREEN_HEIGHT * 3);

    // 重置批次渲染器
    if (g_worldBatch)
    {
        Batch_DestroyRenderer(g_worldBatch);
    }
    g_worldBatch = Batch_CreateRenderer(4096, 12288);

    // 清空角色池和子弹池
    CharacterPool_Clear(&g_characterPool);
    BulletPool_Clear(&g_bulletPool);

    // 创建玩家角色
    CreatePlayerCharacter();

    // 创建测试角色(for debug)
    // CreateTestCharacters();

    // 先创建一次敌人
    generateEnemyCharacter();

    // 重置控制状态
    memset(&g_gameControls, 0, sizeof(g_gameControls));

    // 重置帧控制器
    FrameController_Init(&g_frameController, LOGIC_FRAME_RATE);

    // 重置游玩时间
    playSceneTime = 0;
    start = SDL_GetPerformanceCounter();
    now = start;

    // 重置是否在选择武器
    select = false;

    // 重置分数
    score = 0;

    // 重置刷怪
    lastSpawnTime = 0;
    spawnInterval = 5.0f;
}

void GamePlayScene_Cleanup(void)
{
    SDL_Log("Cleaning up Game Play scene");
    // 游戏场景清理时可以保留角色池数据，或根据需要清空
    if (score > maxScore) maxScore = score;
}

SDL_AppResult GamePlayScene_Event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        ChangeScene(SCENE_EXIT);
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE)
        {
            // 按ESC返回主菜单
            ChangeScene(SCENE_MAIN_MENU);
            return SDL_APP_CONTINUE;
        }

        // 相机控制
        if (event->key.key == SDLK_UP) g_gameControls.cameraMoveUP = true;
        if (event->key.key == SDLK_DOWN) g_gameControls.cameraMoveDown = true;
        if (event->key.key == SDLK_LEFT) g_gameControls.cameraMoveLeft = true;
        if (event->key.key == SDLK_RIGHT) g_gameControls.cameraMoveRight = true;

        // 角色控制
        if (event->key.key == SDLK_W) g_gameControls.moveForward = true;
        if (event->key.key == SDLK_A) g_gameControls.turnLeft = true;
        if (event->key.key == SDLK_D) g_gameControls.turnRight = true;

        break;

    case SDL_EVENT_KEY_UP:
        // 相机控制
        if (event->key.key == SDLK_UP) g_gameControls.cameraMoveUP = false;
        if (event->key.key == SDLK_DOWN) g_gameControls.cameraMoveDown = false;
        if (event->key.key == SDLK_LEFT) g_gameControls.cameraMoveLeft = false;
        if (event->key.key == SDLK_RIGHT) g_gameControls.cameraMoveRight = false;

        // 角色控制
        if (event->key.key == SDLK_W) g_gameControls.moveForward = false;
        if (event->key.key == SDLK_A) g_gameControls.turnLeft = false;
        if (event->key.key == SDLK_D) g_gameControls.turnRight = false;
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) g_gameControls.headShoot = true;
        if (event->button.button == SDL_BUTTON_RIGHT) g_gameControls.tailShoot = true;
        if (event->button.button == SDL_BUTTON_LEFT && select)
        {
            if (PointInRect(event->button.x, event->button.y, (SDL_FRect){SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 200}))
            {
                g_playerCharacter->headGun.bullet = selectedBullet;
                select = false;
            }
            else if (PointInRect(event->button.x, event->button.y, (SDL_FRect){SCREEN_WIDTH * 0.75f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 200}))
            {
                Bullet oldBullet = g_playerCharacter->headGun.bullet;
                g_playerCharacter->headGun = selectedGun;
                g_playerCharacter->headGun.bullet = oldBullet;
                select = false;
            }
        }
        if (event->button.button == SDL_BUTTON_RIGHT && select)
        {
            if (PointInRect(event->button.x, event->button.y, (SDL_FRect){SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 200}))
            {
                g_playerCharacter->tailGun.bullet = selectedBullet;
                select = false;
            }
            else if (PointInRect(event->button.x, event->button.y, (SDL_FRect){SCREEN_WIDTH * 0.75f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 200}))
            {
                Bullet oldBullet = g_playerCharacter->tailGun.bullet;
                g_playerCharacter->tailGun = selectedGun;
                g_playerCharacter->tailGun.bullet = oldBullet;
                select = false;
            }
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) g_gameControls.headShoot = false;
        if (event->button.button == SDL_BUTTON_RIGHT) g_gameControls.tailShoot = false;
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        if (g_camera)
        {
            if (event->wheel.y > 0)
            {
                Camera_Zoom_Add(g_camera, 0.2f);
            }
            else if (event->wheel.y < 0)
            {
                Camera_Zoom_Add(g_camera, -0.2f);
            }
        }
        break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult GamePlayScene_Update(void)
{
    // 使用帧控制器进行逻辑更新
    int updates = FrameController_Update(&g_frameController);

    // 记录游戏时间
    now = SDL_GetPerformanceCounter();
    playSceneTime = (now - start) / (float)SDL_GetPerformanceFrequency();

    for (int i = 0; i < updates; i++)
    {
        // 摄像机移动处理(for debug)
        if (g_gameControls.cameraMoveUP) Camera_Move(g_camera, 0, 50);
        if (g_gameControls.cameraMoveDown) Camera_Move(g_camera, 0, -50);
        if (g_gameControls.cameraMoveLeft) Camera_Move(g_camera, -50, 0);
        if (g_gameControls.cameraMoveRight) Camera_Move(g_camera, 50, 0);
        // 摄像机跟随玩家
        Camera_Move(g_camera, (g_playerCharacter->body[0].x - g_camera->x) * 0.02, (g_playerCharacter->body[0].y - g_camera->y) * 0.02);
        // 更新玩家角色
        if (g_playerCharacter)
        {
            if (g_gameControls.moveForward)
            {
                if (g_gameControls.turnLeft) Character_turn_left(g_playerCharacter);
                if (g_gameControls.turnRight) Character_turn_right(g_playerCharacter);
                Character_speed_add(g_playerCharacter, 2);
            }
            else
            {
                if (g_playerCharacter->speed > 0) Character_speed_add(g_playerCharacter, -5);
                if (g_playerCharacter->speed < 0) Character_speed_set(g_playerCharacter, 0);
            }

            // 射击
            if (g_playerCharacter->haveHeadGun) Gun_Try_Shoot(&g_playerCharacter->headGun, &g_bulletPool, g_gameControls.headShoot);
            if (g_playerCharacter->haveTailGun) Gun_Try_Shoot(&g_playerCharacter->tailGun, &g_bulletPool, g_gameControls.tailShoot);
        }

        for (int i = 0; i < g_playerCharacter->bodyCount; i++)
        {
            AABBBox bodyBox = {g_playerCharacter->body[i].x - g_playerCharacter->body[i].radius, g_playerCharacter->body[i].x + g_playerCharacter->body[i].radius, g_playerCharacter->body[i].y - g_playerCharacter->body[i].radius, g_playerCharacter->body[i].y + g_playerCharacter->body[i].radius};
            if (AABBBoxCollision(bodyBox, wallUpBox))
            {
                g_playerCharacter->body[i].y -= g_playerCharacter->body[i].radius * 0.5f;
            }
            if (AABBBoxCollision(bodyBox, wallDownBox))
            {
                g_playerCharacter->body[i].y += g_playerCharacter->body[i].radius * 0.5f;
            }
            if (AABBBoxCollision(bodyBox, wallLeftBox))
            {
                g_playerCharacter->body[i].x += g_playerCharacter->body[i].radius * 0.5f;
            }
            if (AABBBoxCollision(bodyBox, wallRightBox))
            {
                g_playerCharacter->body[i].x -= g_playerCharacter->body[i].radius * 0.5f;
            }
        }
        // 敌人ai:面朝玩家

        for (int i = 1; i < g_characterPool.size; i++)
        {
            Vector enemyToPlayer = vector_get(g_characterPool.characters[i]->body[0].x, g_characterPool.characters[i]->body[0].y, g_playerCharacter->body[0].x, g_playerCharacter->body[0].y);
            Character_turn_to_vector(g_characterPool.characters[i], enemyToPlayer);
        }

        // 更新所有角色
        CharacterPool_Update(&g_characterPool);

        // 更新所有子弹
        BulletPool_Update(&g_bulletPool, &g_characterPool);

        // 检查所有怪物角色血量
        CharacterPool_Check_Enemy_HP(&g_characterPool, &score);

        // 检查游戏结束条件（示例：玩家生命值低于0）
        if (g_playerCharacter && g_playerCharacter->HP <= 0)
        {
            ChangeScene(SCENE_GAME_OVER);
            break;
        }

        // 刷怪逻辑：随着游戏时间推移，逐渐增加刷怪频率

        // 每30秒降低刷怪间隔（加快刷怪速度），最低间隔为1秒
        int newSpawnInterval = SDL_max(1, 5 - (int)(playSceneTime / 30));
        if (newSpawnInterval != spawnInterval)
        {
            spawnInterval = newSpawnInterval;
        }

        // 如果到了刷怪时间，生成怪物
        if (playSceneTime - lastSpawnTime >= spawnInterval)
        {
            generateEnemyCharacter();
            generateEnemyCharacter();
            lastSpawnTime = playSceneTime;
        }

        // 升级武器
        if (fmodf(playSceneTime, 10) >= 9.8)
        {
            select = true;
            srand(time(NULL));
            enum BulletType BulletRand = rand() % 3;
            enum GunType GunRand = rand() % 3;
            if (BulletRand == AMMO)
            {
                selectedBullet = ammo;
                selectedBulletType = AMMO;
            }
            else if (BulletRand == BUBBLE)
            {
                selectedBullet = bubble;
                selectedBulletType = BUBBLE;
            }
            else if (BulletRand == SNIPERBULLET)
            {
                selectedBullet = sniperBullet;
                selectedBulletType = SNIPERBULLET;
            }
            if (GunRand == SHORTGUN)
            {
                selectedGun = shortGun;
            }
            else if (GunRand == LONGGUN)
            {
                selectedGun = longGun;
            }
            else if (GunRand == SNIPERGUN)
            {
                selectedGun = sniperGun;
            }
        }
    }

    return SDL_APP_CONTINUE;
}

static void DrawCameraViewRectBorder(const Camera *camera);
void GamePlayScene_Render(void)
{
    // 清除屏幕
    SDL_SetRenderDrawColor(g_renderer, 30, 30, 40, 255);
    SDL_RenderClear(g_renderer);

    // 渲染网格背景
    RenderGridBackground(g_camera, g_renderer);

    // 世界批次
    if (g_worldBatch)
    {
        // 先清空批次
        Batch_Clear(g_worldBatch);

        // 渲染墙
        Polygon_DrawRect(g_worldBatch, wallUpRect.x, wallUpRect.y, wallUpRect.w, wallUpRect.h, white, g_camera);
        Polygon_DrawRect(g_worldBatch, wallDownRect.x, wallDownRect.y, wallDownRect.w, wallDownRect.h, white, g_camera);
        Polygon_DrawRect(g_worldBatch, wallLeftRect.x, wallLeftRect.y, wallLeftRect.w, wallLeftRect.h, white, g_camera);
        Polygon_DrawRect(g_worldBatch, wallRightRect.x, wallRightRect.y, wallRightRect.w, wallRightRect.h, white, g_camera);

        // 渲染所有角色
        CharacterPool_Render(&g_characterPool, g_renderer, g_camera, g_worldBatch);

        // 渲染子弹
        BulletPool_Render(&g_bulletPool, g_renderer, g_camera, g_worldBatch);

        //  渲染相机视野边框（for debug）
        // DrawCameraViewRectBorder(g_camera);
        // 渲染批次
        if (!Batch_IsEmpty(g_worldBatch))
        {
            Batch_Render(g_worldBatch, g_renderer);
        }
    }
    // 渲染角色拥有的枪
    if (g_playerCharacter->haveHeadGun)
    {
        Gun_render(&g_playerCharacter->headGun, g_playerCharacter->headGun.direction, g_renderer, g_camera, textures);
    }
    if (g_playerCharacter->haveTailGun)
    {
        Gun_render(&g_playerCharacter->tailGun, g_playerCharacter->tailGun.direction, g_renderer, g_camera, textures);
    }
    // 渲染UI（游戏内UI）
    if (g_uiManager)
    {
        UI_BeginDraw(g_uiManager);

        SDL_FRect HPRect = {10, SCREEN_HEIGHT - 40, 390, 30};
        SDL_FRect HPtextRect = {HPRect.x, HPRect.y, HPRect.w * 0.5f, HPRect.h};
        // 绘制血条矩行
        if (g_playerCharacter)
        {
            float HPPercent = g_playerCharacter->HP / g_playerCharacter->maxHP;
            UI_DrawRect(g_uiManager, HPRect.x, HPRect.y, HPRect.w, HPRect.h, (SDL_FColor){0.3f, 0.3f, 0.3f, 0.8f}, true);
            UI_DrawRect(g_uiManager, HPRect.x, HPRect.y, HPRect.w * HPPercent, HPRect.h, (SDL_FColor){1.0f, 0.0f, 0.0f, 0.8f}, true);
        }

        if (select)
        {
            // 渲染选择信息框
            UI_DrawRect(g_uiManager, SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 200, white, true);
            UI_DrawRect(g_uiManager, SCREEN_WIDTH * 0.75f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 200, white, true);
            // 渲染选择纹理
            // 子弹用文本代替
            Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f - 300, 300, 50}, darkGold, selectedBulletName[selectedBulletType]);
            // 枪的纹理

            Render_Texture(g_renderer, textures[selectedGun.type], (SDL_FPoint){SCREEN_WIDTH * 0.75f, SCREEN_HEIGHT * 0.75f - 300}, 0, 10);
        }

        UI_EndDraw(g_uiManager, g_renderer);

        // 绘制文本
        if (g_playerCharacter)
        {
            // 绘制血条文本
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%d/%d", (int)roundf(g_playerCharacter->HP), (int)roundf(g_playerCharacter->maxHP));
            Draw_Text(g_font, g_renderer, HPtextRect, white, buffer);

            // 绘制分数文本
            snprintf(buffer, sizeof(buffer), "Score: %d", score);
            Draw_Text(g_font, g_renderer, scoreTextRect, white, buffer);

            // 绘制时间文本
            snprintf(buffer, sizeof(buffer), "Survival Time: %.2fs", playSceneTime);
            Draw_Text(g_font, g_renderer, timeTextRect, white, buffer);

            // 渲染选择信息
            if (select)
            {
                // 左边:子弹
                snprintf(buffer, sizeof(buffer), "Bullet");
                Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 50}, darkGold, buffer);
                snprintf(buffer, sizeof(buffer), "damage:%.1f", selectedBullet.damage);
                Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f - 50, 300, 50}, darkGold, buffer);
                snprintf(buffer, sizeof(buffer), "radius:%.1f", selectedBullet.radius);
                Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f, 300, 50}, darkGold, buffer);
                snprintf(buffer, sizeof(buffer), "fly-speed:%.1f", selectedBullet.speed);
                Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.25f - 150, SCREEN_HEIGHT * 0.75f + 50, 300, 50}, darkGold, buffer);

                // 右边:枪
                snprintf(buffer, sizeof(buffer), "Gun");
                Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.75f - 150, SCREEN_HEIGHT * 0.75f - 100, 300, 50}, darkGold, buffer);
                snprintf(buffer, sizeof(buffer), "RPM:%.1f", 60.0f * (LOGIC_FRAME_RATE / selectedGun.shootDelay));
                Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.75f - 150, SCREEN_HEIGHT * 0.75f - 50, 300, 50}, darkGold, buffer);
                snprintf(buffer, sizeof(buffer), "damage-factor:%.1f", selectedGun.damageK);
                Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.75f - 150, SCREEN_HEIGHT * 0.75f, 300, 50}, darkGold, buffer);
            }
            // Draw_Text(g_font, g_renderer, (SDL_FRect){SCREEN_WIDTH * 0.25f - 300, SCREEN_HEIGHT * 0.75f - 100, 300, 200}, darkGold, buffer);
        }
    }

    // 渲染FPS
    FrameController_UpdateFPS(&g_frameController, &FPS, &UPS);
    RenderFPSDisplay(FPS, UPS);

    SDL_RenderPresent(g_renderer);
    FrameController_AddRenderCount(&g_frameController);
}

// ==================== 游戏结束场景实现 ====================

void GameOverScene_Init(void)
{
    SDL_Log("Initializing Game Over scene");

    // 重置UI管理器
    if (g_uiManager)
    {
        UI_DestroyManager(g_uiManager);
    }
    g_uiManager = UI_CreateManager();

    // 重置相机
    if (g_camera)
    {
        Camera_Destroy(g_camera);
    }
    g_camera = Camera_Create(0.0f, 0.0f, LOGICAL_WIDTH, LOGICAL_HEIGHT);

    // 重置批次渲染器
    if (g_worldBatch)
    {
        Batch_DestroyRenderer(g_worldBatch);
    }
    g_worldBatch = Batch_CreateRenderer(2048, 6144);

    // 重置帧控制器
    FrameController_Init(&g_frameController, LOGIC_FRAME_RATE);
}

void GameOverScene_Cleanup(void) { SDL_Log("Cleaning up Game Over scene"); }

SDL_AppResult GameOverScene_Event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        ChangeScene(SCENE_EXIT);
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE)
        {
            ChangeScene(SCENE_EXIT);
            return SDL_APP_SUCCESS;
        }

        // 按R键重新开始
        if (event->key.key == SDLK_R)
        {
            ChangeScene(SCENE_GAME_PLAY);
            return SDL_APP_CONTINUE;
        }

        // 按M键返回主菜单
        if (event->key.key == SDLK_M)
        {
            ChangeScene(SCENE_MAIN_MENU);
            return SDL_APP_CONTINUE;
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // 重新开始按钮
        if (PointInRect(event->button.x, event->button.y, restartRect))
        {
            ChangeScene(SCENE_GAME_PLAY);
        }

        // 主菜单按钮
        if (PointInRect(event->button.x, event->button.y, toMeneRect))
        {
            ChangeScene(SCENE_MAIN_MENU);
        }
        break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult GameOverScene_Update(void)
{
    FrameController_Update(&g_frameController);
    return SDL_APP_CONTINUE;
}

void GameOverScene_Render(void)
{
    // 清除屏幕
    SDL_SetRenderDrawColor(g_renderer, 50, 30, 30, 255);
    SDL_RenderClear(g_renderer);

    // 渲染背景
    if (g_worldBatch)
    {
        Batch_Clear(g_worldBatch);

        // 绘制红色背景方块
        for (int i = 0; i < 10; i++)
        {
            float x = (i % 5) * 150 - 300;
            float y = (i / 5.0f) * 150 - 150;
            SDL_FColor color = {0.5f, 0.1f + i * 0.02f, 0.1f + i * 0.01f, 0.7f};
            Polygon_DrawRect(g_worldBatch, x, y, 140, 140, color, g_camera);
        }

        Batch_Render(g_worldBatch, g_renderer);
    }

    // 渲染UI
    if (g_uiManager)
    {
        UI_BeginDraw(g_uiManager);

        // 游戏结束标题
        UI_DrawRect(g_uiManager, gameoverTitleRect.x, gameoverTitleRect.y, gameoverTitleRect.w, gameoverTitleRect.h, (SDL_FColor){0.6f, 0.2f, 0.2f, 0.9f}, true);

        // 重新开始按钮
        UI_DrawRect(g_uiManager, restartRect.x, restartRect.y, restartRect.w, restartRect.h, (SDL_FColor){0.3f, 0.7f, 0.3f, 0.9f}, true);

        // 主菜单按钮
        UI_DrawRect(g_uiManager, toMeneRect.x, toMeneRect.y, toMeneRect.w, toMeneRect.h, (SDL_FColor){0.3f, 0.3f, 0.7f, 0.9f}, true);

        UI_EndDraw(g_uiManager, g_renderer);
    }

    // 渲染文字
    if (g_font)
    {

        // 游戏结束文字
        Draw_Text(g_font, g_renderer, gameoverTitleRect, white, "GAME OVER");

        // 重新开始文字
        Draw_Text(g_font, g_renderer, restartRect, white, "RESTART (R)");

        // 主菜单文字
        Draw_Text(g_font, g_renderer, toMeneRect, white, "MENU (M)");

        char buffer[32];
        // 分数文字
        SDL_FRect scoreTextRect = {SCREEN_WIDTH / 2.0f - 150, 200, 300, 60};
        snprintf(buffer, sizeof(buffer), "Score: %d", score);
        Draw_Text(g_font, g_renderer, scoreTextRect, white, buffer);
        // 最高分
        snprintf(buffer, sizeof(buffer), "MAX Score: %d", maxScore);
        scoreTextRect.y += 50;
        Draw_Text(g_font, g_renderer, scoreTextRect, white, buffer);

        // 存活时间文字
        SDL_FRect TimeTextRect = {SCREEN_WIDTH / 2.0f - 150, 300, 300, 60};

        snprintf(buffer, sizeof(buffer), "Survival Time: %.1fs", playSceneTime);
        Draw_Text(g_font, g_renderer, TimeTextRect, white, buffer);
    }

    // 渲染FPS
    FrameController_UpdateFPS(&g_frameController, &FPS, &UPS);
    RenderFPSDisplay(FPS, UPS);

    SDL_RenderPresent(g_renderer);
    FrameController_AddRenderCount(&g_frameController);
}

static bool g_isRunning = true;
Character *testCharacter = NULL;

// 绘制摄像机可见范围矩形的边框(for debug)
static void DrawCameraViewRectBorder(const Camera *camera)
{
    if (!camera) return;

    AABBBox viewBox = Rect_To_AABBBox(Camera_GetViewRect(camera));
    SDL_FColor borderColor = {1.0f, 0.8f, 0.2f, 1.0f}; // 金黄色边框
    Polygon_DrawLine(g_worldBatch, viewBox.minX, viewBox.minY, viewBox.maxX, viewBox.minY, 10.0f / camera->zoom, borderColor, camera);
    Polygon_DrawLine(g_worldBatch, viewBox.minX, viewBox.minY, viewBox.minX, viewBox.maxY, 10.0f / camera->zoom, borderColor, camera);
    Polygon_DrawLine(g_worldBatch, viewBox.minX, viewBox.maxY, viewBox.maxX, viewBox.maxY, 10.0f / camera->zoom, borderColor, camera);
    Polygon_DrawLine(g_worldBatch, viewBox.maxX, viewBox.minY, viewBox.maxX, viewBox.maxY, 10.0f / camera->zoom, borderColor, camera);
}

// SDL3应用程序初始化回调
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    printf("=== 应用程序初始化开始 ===\n");

    // 初始化SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("SDL初始化失败: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // 创建窗口
    g_window = SDL_CreateWindow("场景切换演示 - 主菜单 | 游戏 | 失败画面", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!g_window)
    {
        printf("窗口创建失败: %s\n", SDL_GetError());
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    // 创建渲染器
    g_renderer = SDL_CreateRenderer(g_window, NULL);
    if (!g_renderer)
    {
        printf("渲染器创建失败: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    // 设置逻辑渲染尺寸
    SDL_SetRenderLogicalPresentation(g_renderer, LOGICAL_WIDTH, LOGICAL_HEIGHT, SDL_LOGICAL_PRESENTATION_STRETCH);

    // 初始化TTF
    if (!TTF_Init())
    {
        printf("TTF初始化失败\n");
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    // 加载字体
    g_font = TTF_OpenFont("../font/fengwujiutian.ttf", 16);

    // 加载纹理
    shortGunTexture = IMG_LoadTexture(g_renderer, "../image/shortGun.png");
    longGunTexture = IMG_LoadTexture(g_renderer, "../image/longGun.png");
    sniperGunTexture = IMG_LoadTexture(g_renderer, "../image/sniperGun.png");
    SDL_SetTextureScaleMode(shortGunTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(longGunTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(sniperGunTexture, SDL_SCALEMODE_NEAREST);
    textures[0] = shortGunTexture;
    textures[1] = longGunTexture;
    textures[2] = sniperGunTexture;

    // 创建墙矩行
    wallUpRect = AABBBox_To_Rect(wallUpBox);
    wallDownRect = AABBBox_To_Rect(wallDownBox);
    wallLeftRect = AABBBox_To_Rect(wallLeftBox);
    wallRightRect = AABBBox_To_Rect(wallRightBox);

    // 初始化角色池
    CharacterPool_Init(&g_characterPool, 100);

    // 初始化子弹池
    BulletPool_Init(&g_bulletPool);

    // 设置窗口位置
    SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // 加载最高分
    FILE *fp = fopen("../data/data.txt", "r");
    if (fp)
    {
        fscanf(fp, "%d", &maxScore);
    }
    fclose(fp);

    printf("=== 应用程序初始化成功 ===\n");
    printf("初始场景: 主菜单\n");

    return SDL_APP_CONTINUE;
}
// SDL3事件处理回调
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // 处理场景切换
    if (g_sceneChanged)
    {
        // 检查退出场景
        if (g_currentScene == SCENE_EXIT)
        {
            return SDL_APP_SUCCESS;
        }

        // 调用旧场景的清理函数
        if (g_scenes[g_currentScene].cleanup)
        {
            g_scenes[g_currentScene].cleanup();
        }

        // 切换到新场景
        g_currentScene = g_nextScene;
        g_sceneChanged = false;

        // 调用新场景的初始化函数
        if (g_scenes[g_currentScene].init)
        {
            g_scenes[g_currentScene].init();
        }

        char sceneName[3][10] = {"主菜单", "游戏", "失败"};
        SDL_Log("切换到场景: %s", sceneName[g_scenes[g_currentScene].id]);
    }

    // 将事件传递给当前场景
    if (g_scenes[g_currentScene].handle_event)
    {
        return g_scenes[g_currentScene].handle_event(event);
    }

    return SDL_APP_CONTINUE;
}

// SDL3主循环回调
SDL_AppResult SDL_AppIterate(void *appstate)
{
    // 检查退出场景
    if (g_currentScene == SCENE_EXIT)
    {
        return SDL_APP_SUCCESS;
    }

    // 更新当前场景
    if (g_scenes[g_currentScene].update)
    {
        SDL_AppResult result = g_scenes[g_currentScene].update();
        if (result != SDL_APP_CONTINUE)
        {
            return result;
        }
    }

    // 渲染当前场景
    if (g_scenes[g_currentScene].render)
    {
        g_scenes[g_currentScene].render();
    }

    return SDL_APP_CONTINUE;
}

// SDL3应用程序清理回调
void SDL_AppQuit(void *appstate, SDL_AppResult reason)
{
    printf("=== 应用程序清理开始 ===\n");
    printf("退出原因: %s\n", (reason == SDL_APP_SUCCESS) ? "正常退出" : "初始化失败");

    // 清理资源
    if (g_uiManager) UI_DestroyManager(g_uiManager);
    if (g_worldBatch) Batch_DestroyRenderer(g_worldBatch);
    if (g_camera) Camera_Destroy(g_camera);
    if (g_font) TTF_CloseFont(g_font);

    // 销毁所有角色和子弹
    CharacterPool_Destroy(&g_characterPool);
    BulletPool_Destroy(&g_bulletPool);

    // 销毁SDL资源
    // 销毁纹理
    SDL_DestroyTexture(shortGunTexture);
    SDL_DestroyTexture(longGunTexture);
    SDL_DestroyTexture(sniperGunTexture);
    if (g_renderer)
    {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
        printf("渲染器已销毁\n");
    }

    if (g_window)
    {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        printf("窗口已销毁\n");
    }

    // 退出库
    TTF_Quit();
    SDL_Quit();
    FILE *fp = fopen("../data/data.txt", "w");
    if (!fp) return;
    fprintf(fp, "%d\n", maxScore);
    fclose(fp);
    printf("=== 应用程序清理完成 ===\n");
}