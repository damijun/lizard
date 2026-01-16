#include "batchingRender.h"
#include <stdlib.h>
#include <string.h>

// 默认初始容量
#define DEFAULT_VERTEX_CAPACITY 1024
#define DEFAULT_INDEX_CAPACITY 3072 // 假设每个三角形3个索引，1024个三角形

// 创建批次渲染器
BatchRenderer *Batch_CreateRenderer(int initialVertexCapacity, int initialIndexCapacity)
{
    BatchRenderer *batch = (BatchRenderer *)malloc(sizeof(BatchRenderer));
    if (!batch) return NULL;

    // 设置初始容量
    if (initialVertexCapacity <= 0) initialVertexCapacity = DEFAULT_VERTEX_CAPACITY;
    if (initialIndexCapacity <= 0) initialIndexCapacity = DEFAULT_INDEX_CAPACITY;

    // 分配顶点数组
    batch->vertices = (SDL_Vertex *)malloc(sizeof(SDL_Vertex) * initialVertexCapacity);
    if (!batch->vertices)
    {
        free(batch);
        return NULL;
    }

    // 分配索引数组
    batch->indices = (int *)malloc(sizeof(int) * initialIndexCapacity);
    if (!batch->indices)
    {
        free(batch->vertices);
        free(batch);
        return NULL;
    }

    // 初始化成员
    batch->vertexCount = 0;
    batch->vertexCapacity = initialVertexCapacity;
    batch->indexCount = 0;
    batch->indexCapacity = initialIndexCapacity;

    return batch;
}

// 销毁批次渲染器
void Batch_DestroyRenderer(BatchRenderer *batch)
{
    if (!batch) return;
    if (batch->vertices) free(batch->vertices);
    if (batch->indices) free(batch->indices);
    free(batch);
}

// 清空批次数据
void Batch_Clear(BatchRenderer *batch)
{
    if (!batch) return;

    batch->vertexCount = 0;
    batch->indexCount = 0;
}

// 确保顶点数组有足够空间
static bool Batch_EnsureVertexCapacity(BatchRenderer *batch, int requiredCapacity)
{
    if (!batch || requiredCapacity <= 0) return false;

    if (requiredCapacity <= batch->vertexCapacity) return true;

    // 计算新的容量（按2倍增长）
    int newCapacity = batch->vertexCapacity;
    while (newCapacity < requiredCapacity)
    {
        newCapacity = newCapacity * 2;
    }

    // 重新分配内存
    SDL_Vertex *newVertices = (SDL_Vertex *)realloc(batch->vertices, sizeof(SDL_Vertex) * newCapacity);
    if (!newVertices) return false;

    batch->vertices = newVertices;
    batch->vertexCapacity = newCapacity;
    return true;
}

// 确保索引数组有足够空间
static bool Batch_EnsureIndexCapacity(BatchRenderer *batch, int requiredCapacity)
{
    if (!batch || requiredCapacity <= 0) return false;

    if (requiredCapacity <= batch->indexCapacity) return true;

    // 计算新的容量（按2倍增长）
    int newCapacity = batch->indexCapacity;
    while (newCapacity < requiredCapacity)
    {
        newCapacity = newCapacity * 2;
    }

    // 重新分配内存
    int *newIndices = (int *)realloc(batch->indices, sizeof(int) * newCapacity);
    if (!newIndices) return false;

    batch->indices = newIndices;
    batch->indexCapacity = newCapacity;
    return true;
}

// 添加顶点到批次（屏幕坐标）
int Batch_AddVertex(BatchRenderer *batch, const SDL_Vertex *vertex)
{
    if (!batch || !vertex) return -1;

    // 确保有足够空间
    if (!Batch_EnsureVertexCapacity(batch, batch->vertexCount + 1))
    {
        return -1;
    }

    // 复制顶点数据
    batch->vertices[batch->vertexCount] = *vertex;

    // 返回顶点索引
    return batch->vertexCount++;
}

// 添加索引到批次
bool Batch_AddIndex(BatchRenderer *batch, int index)
{
    if (!batch || index < 0) return false;

    // 确保有足够空间
    if (!Batch_EnsureIndexCapacity(batch, batch->indexCount + 1))
    {
        return false;
    }

    // 添加索引
    batch->indices[batch->indexCount++] = index;
    return true;
}

// 直接添加一个三角形到批次（3个顶点，3个索引）
// 注意：这里的点坐标必须是屏幕坐标！
bool Batch_AddTriangle(BatchRenderer *batch, const SDL_FPoint *p1, const SDL_FPoint *p2, const SDL_FPoint *p3, SDL_FColor color)
{
    if (!batch || !p1 || !p2 || !p3) return false;

    // 确保有足够空间添加3个顶点和3个索引
    if (!Batch_EnsureVertexCapacity(batch, batch->vertexCount + 3) || !Batch_EnsureIndexCapacity(batch, batch->indexCount + 3))
    {
        return false;
    }

    // 创建三个顶点
    SDL_Vertex vertices[3];

    vertices[0].position.x = p1->x;
    vertices[0].position.y = p1->y;
    vertices[0].color = color;
    vertices[0].tex_coord.x = 0.0f; // 不使用纹理，设为0
    vertices[0].tex_coord.y = 0.0f;

    vertices[1].position.x = p2->x;
    vertices[1].position.y = p2->y;
    vertices[1].color = color;
    vertices[1].tex_coord.x = 0.0f;
    vertices[1].tex_coord.y = 0.0f;

    vertices[2].position.x = p3->x;
    vertices[2].position.y = p3->y;
    vertices[2].color = color;
    vertices[2].tex_coord.x = 0.0f;
    vertices[2].tex_coord.y = 0.0f;

    // 添加顶点并获取索引
    int baseIndex = batch->vertexCount;
    for (int i = 0; i < 3; i++)
    {
        batch->vertices[batch->vertexCount++] = vertices[i];
    }

    // 添加三角形索引（0, 1, 2）
    batch->indices[batch->indexCount++] = baseIndex;
    batch->indices[batch->indexCount++] = baseIndex + 1;
    batch->indices[batch->indexCount++] = baseIndex + 2;

    return true;
}

// 渲染整个批次（单次drawcall）
bool Batch_Render(BatchRenderer *batch, SDL_Renderer *renderer)
{
    if (!batch || !renderer || batch->vertexCount == 0) return false;

    // 注意：批次中的顶点已经是屏幕坐标，不需要应用相机变换！

    // 渲染几何体
    bool success = false;

    if (batch->indexCount > 0)
    {
        // 使用索引渲染
        success = SDL_RenderGeometry(renderer,
                                     NULL, // 没有纹理
                                     batch->vertices, batch->vertexCount, batch->indices, batch->indexCount);
    }
    else
    {
        // 如果没有索引，则按三角形列表渲染（每3个顶点一个三角形）
        success = SDL_RenderGeometry(renderer,
                                     NULL, // 没有纹理
                                     batch->vertices, batch->vertexCount, NULL, 0);
    }

    return success;
}

// 获取批次统计信息
void Batch_GetStats(const BatchRenderer *batch, int *vertexCount, int *indexCount, int *triangleCount)
{
    if (vertexCount) *vertexCount = batch ? batch->vertexCount : 0;
    if (indexCount) *indexCount = batch ? batch->indexCount : 0;

    if (triangleCount)
    {
        if (batch && batch->indexCount > 0)
        {
            // 如果有索引，每个三角形3个索引
            *triangleCount = batch->indexCount / 3;
        }
        else if (batch)
        {
            // 如果没有索引，每3个顶点一个三角形
            *triangleCount = batch->vertexCount / 3;
        }
        else
        {
            *triangleCount = 0;
        }
    }
}

// 检查批次是否为空
bool Batch_IsEmpty(const BatchRenderer *batch) { return !batch || (batch->vertexCount == 0 && batch->indexCount == 0); }