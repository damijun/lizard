#include "ui.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ui所有像素位置都是屏幕位置

// 创建UI管理器
UIManager *UI_CreateManager(void)
{
    UIManager *manager = (UIManager *)malloc(sizeof(UIManager));
    if (!manager) return NULL;

    manager->uiBatch = Batch_CreateRenderer(512, 1536); // 初始容量
    manager->elementCount = 0;

    return manager;
}

// 销毁UI管理器
void UI_DestroyManager(UIManager *manager)
{
    if (manager)
    {
        if (manager->uiBatch) Batch_DestroyRenderer(manager->uiBatch);
        free(manager);
    }
}

// 开始UI绘制（清空批次）
void UI_BeginDraw(UIManager *manager)
{
    if (!manager || !manager->uiBatch) return;

    Batch_Clear(manager->uiBatch);
    manager->elementCount = 0;
}

// 结束UI绘制（渲染批次）
void UI_EndDraw(UIManager *manager, SDL_Renderer *renderer)
{
    if (!manager || !manager->uiBatch || !renderer) return;

    if (!Batch_IsEmpty(manager->uiBatch))
    {
        Batch_Render(manager->uiBatch, renderer);
    }
}

// 绘制UI矩形（填充）
void UI_DrawRect(UIManager *manager, float x, float y, float width, float height, SDL_FColor color, bool filled)
{
    if (!manager || !manager->uiBatch) return;

    if (filled)
    {
        // 使用批处理绘制填充矩形
        SDL_FPoint points[4] = {{x, y}, {x + width, y}, {x + width, y + height}, {x, y + height}};

        // 转换为三角形并添加到批次
        // 第一个三角形
        SDL_FPoint p1 = points[0];
        SDL_FPoint p2 = points[1];
        SDL_FPoint p3 = points[2];
        Batch_AddTriangle(manager->uiBatch, &p1, &p2, &p3, color);

        // 第二个三角形
        SDL_FPoint p4 = points[0];
        SDL_FPoint p5 = points[2];
        SDL_FPoint p6 = points[3];
        Batch_AddTriangle(manager->uiBatch, &p4, &p5, &p6, color);

        manager->elementCount++;
    }
    else
    {
        // 绘制边框（不使用批处理，简化实现）
        // 注意：边框绘制不适合批处理，因为需要绘制线条
        // 这里只是示例，实际项目中可能需要单独处理
    }
}

// 绘制UI圆形（填充）
void UI_DrawCircle(UIManager *manager, float centerX, float centerY, float radius, SDL_FColor color, bool filled)
{
    if (!manager || !manager->uiBatch || !filled) return;

    const int segments = 32;
    float angleStep = 2.0f * M_PI / segments;

    // 使用三角形扇形绘制圆形
    for (int i = 0; i < segments; i++)
    {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        SDL_FPoint center = {centerX, centerY};
        SDL_FPoint p1 = {centerX + cosf(angle1) * radius, centerY + sinf(angle1) * radius};
        SDL_FPoint p2 = {centerX + cosf(angle2) * radius, centerY + sinf(angle2) * radius};

        Batch_AddTriangle(manager->uiBatch, &center, &p1, &p2, color);
    }

    manager->elementCount++;
}

// 绘制UI三角形
void UI_DrawTriangle(UIManager *manager, float x1, float y1, float x2, float y2, float x3, float y3, SDL_FColor color)
{
    if (!manager || !manager->uiBatch) return;

    SDL_FPoint p1 = {x1, y1};
    SDL_FPoint p2 = {x2, y2};
    SDL_FPoint p3 = {x3, y3};

    Batch_AddTriangle(manager->uiBatch, &p1, &p2, &p3, color);
    manager->elementCount++;
}

// 屏幕点是否在SDL矩行内
bool PointInRect(float x, float y, SDL_FRect rect) { return x > rect.x && x < rect.x + rect.w && y > rect.y && y < rect.y + rect.h; }

// 绘制文本(如果需要格式化输出,需提前准备文本)
// 比如:
// char buff[100];
// snprintf(buff, sizeof(buff), "Score: %d", score);
void Draw_Text(TTF_Font *font, SDL_Renderer *renderer, SDL_FRect rect, SDL_FColor Fcolor, const char *text)
{
    if (!font || !renderer || !text) return;
    SDL_Color color = {Fcolor.r * 255, Fcolor.g * 255, Fcolor.b * 255, Fcolor.a * 255};
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, strlen(text), color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderTexture(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}