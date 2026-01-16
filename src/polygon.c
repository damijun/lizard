#include "polygon.h"
#include "vector.h"
#include <math.h>
#include <stdlib.h>

// ========== 批处理渲染函数 ==========

// 绘制单个三角形到批次
void Polygon_DrawTriangle(BatchRenderer *batch, float x1, float y1, float x2, float y2, float x3, float y3, SDL_FColor color, const Camera *camera)
{
    if (!batch) return;

    // 关键：将世界坐标转换为屏幕坐标
    SDL_FPoint p1, p2, p3;

    if (camera)
    {
        // 使用相机进行坐标转换
        p1 = Camera_WorldToScreen(camera, x1, y1);
        p2 = Camera_WorldToScreen(camera, x2, y2);
        p3 = Camera_WorldToScreen(camera, x3, y3);
    }
    else
    {
        // 如果没有相机，假设已经是屏幕坐标
        p1.x = x1;
        p1.y = y1;
        p2.x = x2;
        p2.y = y2;
        p3.x = x3;
        p3.y = y3;
    }

    // 将转换后的屏幕坐标添加到批次
    // 注意：Batch_AddTriangle 期望的是屏幕坐标！
    Batch_AddTriangle(batch, &p1, &p2, &p3, color);
}

// 绘制凸多边形到批次（自动三角化）
void Polygon_DrawConvex(BatchRenderer *batch, const SDL_FPoint *points, int pointCount, SDL_FColor color, const Camera *camera)
{
    if (!batch || !points || pointCount < 3) return;

    // 凸多边形三角化：使用三角形扇形
    // 从第一个顶点开始，创建三角形 (0, i, i+1)

    // 为转换后的点分配内存
    SDL_FPoint *transformedPoints = (SDL_FPoint *)malloc(sizeof(SDL_FPoint) * pointCount);
    if (!transformedPoints) return;

    // 转换所有点到屏幕坐标
    if (camera)
    {
        for (int i = 0; i < pointCount; i++)
        {
            transformedPoints[i] = Camera_WorldToScreen(camera, points[i].x, points[i].y);
        }
    }
    else
    {
        // 如果没有相机，直接复制
        memcpy(transformedPoints, points, sizeof(SDL_FPoint) * pointCount);
    }

    // 添加三角形到批次（三角形扇形）
    for (int i = 1; i < pointCount - 1; i++)
    {
        Batch_AddTriangle(batch, &transformedPoints[0], &transformedPoints[i], &transformedPoints[i + 1], color);
    }

    // 清理
    free(transformedPoints);
}

// 绘制矩形到批次（由2个三角形组成）
void Polygon_DrawRect(BatchRenderer *batch, float x, float y, float width, float height, SDL_FColor color, const Camera *camera)
{
    if (!batch) return;

    // 矩形的四个顶点（世界坐标）
    SDL_FPoint worldPoints[4] = {
        {x, y},                  // 左上角
        {x + width, y},          // 右上角
        {x + width, y + height}, // 右下角
        {x, y + height}          // 左下角
    };

    // 第一个三角形：左上角 -> 右上角 -> 右下角
    Polygon_DrawTriangle(batch, worldPoints[0].x, worldPoints[0].y, worldPoints[1].x, worldPoints[1].y, worldPoints[2].x, worldPoints[2].y, color, camera);

    // 第二个三角形：左上角 -> 右下角 -> 左下角
    Polygon_DrawTriangle(batch, worldPoints[0].x, worldPoints[0].y, worldPoints[2].x, worldPoints[2].y, worldPoints[3].x, worldPoints[3].y, color, camera);
}

// 绘制圆形到批次（使用三角形扇形）
void Polygon_DrawCircle(BatchRenderer *batch, float centerX, float centerY, float radius, int segments, SDL_FColor color, const Camera *camera)
{
    if (!batch || segments < 3) return;

    // 生成圆形顶点（世界坐标）
    SDL_FPoint *circlePoints = (SDL_FPoint *)malloc(sizeof(SDL_FPoint) * segments);
    if (!circlePoints) return;

    float angleStep = 2.0f * M_PI / segments;

    for (int i = 0; i < segments; i++)
    {
        float angle = i * angleStep;
        circlePoints[i].x = centerX + cosf(angle) * radius;
        circlePoints[i].y = centerY + sinf(angle) * radius;
    }

    // 使用三角形扇形绘制圆形
    for (int i = 0; i < segments; i++)
    {
        int next = (i + 1) % segments;

        // 每个三角形：圆心 -> 当前点 -> 下一个点
        Polygon_DrawTriangle(batch, centerX, centerY, circlePoints[i].x, circlePoints[i].y, circlePoints[next].x, circlePoints[next].y, color, camera);
    }

    free(circlePoints);
}

// 绘制粗线(矩行);width表示线的实际宽度
four_SDL_FPoint Polygon_DrawLine(BatchRenderer *batch, float x1, float y1, float x2, float y2, float width, SDL_FColor color, const Camera *camera)
{
    float halfWidth = width / 2.0f;
    Vector direction = vector_get(x1, y1, x2, y2);
    Vector verticalDirection_counter = counterclockwise_90(direction);
    Vector verticalDirection = clockwise_90(direction);
    SDL_FPoint p1 = {x1, y1};
    SDL_FPoint p2 = {x2, y2};
    SDL_FPoint p1l = Get_FPoint_From_parametric_equation(p1, verticalDirection_counter, halfWidth);
    SDL_FPoint p1r = Get_FPoint_From_parametric_equation(p1, verticalDirection, halfWidth);
    SDL_FPoint p2l = Get_FPoint_From_parametric_equation(p2, verticalDirection_counter, halfWidth);
    SDL_FPoint p2r = Get_FPoint_From_parametric_equation(p2, verticalDirection, halfWidth);
    Polygon_DrawTriangle(batch, p1l.x, p1l.y, p1r.x, p1r.y, p2l.x, p2l.y, color, camera);
    Polygon_DrawTriangle(batch, p2l.x, p2l.y, p2r.x, p2r.y, p1r.x, p1r.y, color, camera);
    four_SDL_FPoint ret = {p1l, p1r, p2l, p2r};
    return ret;
}
void Polygon_DrawLines(BatchRenderer *batch, SDL_FPoint *pointList, int pointCount, float width, SDL_FColor color, const Camera *camera)
{
    SDL_FPoint lastp2l;
    SDL_FPoint lastp2r;
    for (int i = 0; i < pointCount - 1; i++)
    {
        four_SDL_FPoint points = Polygon_DrawLine(batch, pointList[i].x, pointList[i].y, pointList[i + 1].x, pointList[i + 1].y, width, color, camera);
        // 以下为平滑连接处的操作
        SDL_FPoint p1l = points.p1;
        SDL_FPoint p1r = points.p2;
        SDL_FPoint p2l = points.p3;
        SDL_FPoint p2r = points.p4;
        if (i != 0)
        {
            Polygon_DrawTriangle(batch, lastp2l.x, lastp2l.y, lastp2r.x, lastp2r.y, p1l.x, p1l.y, color, camera);
            Polygon_DrawTriangle(batch, lastp2l.x, lastp2l.y, lastp2r.x, lastp2r.y, p1r.x, p1r.y, color, camera);
            // SDL_Log("%f %f ,%f %f ,%f %f\n", lastp2l.x, lastp2l.y, lastp2r.x, lastp2r.y, p1l.x, p1l.y);
        }
        lastp2l = p2l;
        lastp2r = p2r;
    }
}

// 立即渲染多边形（不使用批处理，保持兼容）
void Polygon_RenderImmediate(SDL_Renderer *renderer, const SDL_FPoint *points, int pointCount, SDL_FColor color, const Camera *camera)
{
    if (!renderer || !points || pointCount < 3) return;

    // 创建临时批次
    BatchRenderer *batch = Batch_CreateRenderer(pointCount * 2, (pointCount - 2) * 3);
    if (!batch) return;

    // 添加到批次
    Polygon_DrawConvex(batch, points, pointCount, color, camera);

    // 渲染并销毁
    Batch_Render(batch, renderer);
    Batch_DestroyRenderer(batch);
}

// ========== 平滑算法部分 ==========

// 获取远处的截取点(截取距离:分点到p2的距离占总距离的比例)
SDL_FPoint Get_far_point(const SDL_FPoint p1, const SDL_FPoint p2, const float cuttingDistance) { return (SDL_FPoint){(1 - cuttingDistance) * p2.x + cuttingDistance * p1.x, (1 - cuttingDistance) * p2.y + cuttingDistance * p1.y}; }