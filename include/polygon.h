#ifndef POLYGON_H
#define POLYGON_H

#include "batchingRender.h"
#include "camera.h"
#include <SDL3/SDL.h>
typedef struct
{
    SDL_FPoint p1, p2, p3, p4;
} four_SDL_FPoint;

// ========== 批处理渲染接口 ==========

// 绘制单个三角形到批次
// 参数说明：
// - batch: 目标批次
// - x1,y1...: 世界坐标
// - color: 三角形颜色
// - camera: 相机（用于世界坐标到屏幕坐标的转换）
void Polygon_DrawTriangle(BatchRenderer *batch, float x1, float y1, float x2, float y2, float x3, float y3, SDL_FColor color, const Camera *camera);

// 绘制凸多边形到批次（自动三角化）
// 注意：points数组必须按顺时针或逆时针顺序排列，且多边形必须是凸的
void Polygon_DrawConvex(BatchRenderer *batch, const SDL_FPoint *points, int pointCount, SDL_FColor color, const Camera *camera);

// 绘制矩形到批次（由2个三角形组成）
void Polygon_DrawRect(BatchRenderer *batch, float x, float y, float width, float height, SDL_FColor color, const Camera *camera);

// 绘制圆形到批次（使用三角形扇形）
void Polygon_DrawCircle(BatchRenderer *batch, float centerX, float centerY, float radius, int segments, SDL_FColor color, const Camera *camera);

// 绘制粗线(矩行);width表示线的实际宽度
four_SDL_FPoint Polygon_DrawLine(BatchRenderer *batch, float x1, float y1, float x2, float y2, float width, SDL_FColor color, const Camera *camera);

// 按顺序绘制粗线数组(矩形);width表示线的实际宽度
void Polygon_DrawLines(BatchRenderer *batch, SDL_FPoint *pointList, int pointCount, float width, SDL_FColor color, const Camera *camera);

// 立即渲染多边形（不使用批处理，保持兼容）
void Polygon_RenderImmediate(SDL_Renderer *renderer, const SDL_FPoint *points, int pointCount, SDL_FColor color, const Camera *camera);

// ========== 平滑算法部分 ==========

// 获取远处的截取点(截取距离:分点到p2的距离占总距离的比例)
SDL_FPoint Get_far_point(const SDL_FPoint p1, const SDL_FPoint p2, const float cuttingDistance);

#endif