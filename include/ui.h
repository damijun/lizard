#ifndef UI_H
#define UI_H

#include "batchingRender.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// UI管理器（简单版本）
typedef struct
{
    BatchRenderer *uiBatch; // UI批处理渲染器
    int elementCount;       // UI元素数量（简单计数）
} UIManager;

// 创建UI管理器
UIManager *UI_CreateManager(void);

// 销毁UI管理器
void UI_DestroyManager(UIManager *manager);

// 开始UI绘制（清空批次）
void UI_BeginDraw(UIManager *manager);

// 结束UI绘制（渲染批次）
void UI_EndDraw(UIManager *manager, SDL_Renderer *renderer);

// 绘制UI矩形
void UI_DrawRect(UIManager *manager, float x, float y, float width, float height, SDL_FColor color, bool filled);

// 绘制UI圆形
void UI_DrawCircle(UIManager *manager, float centerX, float centerY, float radius, SDL_FColor color, bool filled);

// 绘制UI三角形
void UI_DrawTriangle(UIManager *manager, float x1, float y1, float x2, float y2, float x3, float y3, SDL_FColor color);

// 屏幕点是否在SDL矩行内
bool PointInRect(float x, float y, SDL_FRect rect);

void Draw_Text(TTF_Font *font, SDL_Renderer *renderer, SDL_FRect rect, SDL_FColor Fcolor, const char *text);
#endif // UI_H