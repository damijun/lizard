#ifndef BATCHING_RENDER_H
#define BATCHING_RENDER_H

#include <SDL3/SDL.h>
#include <stdbool.h>

// 批次渲染器结构（纯色，无纹理）
typedef struct
{
    SDL_Vertex *vertices; // 顶点数组（存储屏幕坐标）
    int *indices;         // 索引数组
    int vertexCount;      // 当前顶点数量
    int vertexCapacity;   // 顶点数组容量
    int indexCount;       // 当前索引数量
    int indexCapacity;    // 索引数组容量
} BatchRenderer;

// 创建批次渲染器
BatchRenderer *Batch_CreateRenderer(int initialVertexCapacity, int initialIndexCapacity);

// 销毁批次渲染器
void Batch_DestroyRenderer(BatchRenderer *batch);

// 清空批次数据（重用渲染器）
void Batch_Clear(BatchRenderer *batch);

// 添加顶点到批次（屏幕坐标）
// 返回顶点索引，如果失败返回-1
int Batch_AddVertex(BatchRenderer *batch, const SDL_Vertex *vertex);

// 添加索引到批次
bool Batch_AddIndex(BatchRenderer *batch, int index);

// 直接添加一个三角形到批次（3个顶点，3个索引）
// 注意：这里的点坐标必须是屏幕坐标！
bool Batch_AddTriangle(BatchRenderer *batch, const SDL_FPoint *p1, const SDL_FPoint *p2, const SDL_FPoint *p3, SDL_FColor color);

// 渲染整个批次（单次drawcall）
bool Batch_Render(BatchRenderer *batch, SDL_Renderer *renderer);

// 获取批次统计信息
void Batch_GetStats(const BatchRenderer *batch, int *vertexCount, int *indexCount, int *triangleCount);

// 检查批次是否为空
bool Batch_IsEmpty(const BatchRenderer *batch);

#endif // BATCHING_RENDER_H