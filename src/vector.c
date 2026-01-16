#include "vector.h"
#include <SDL3/SDL.h>
#include <math.h>
float vector_norm(Vector v) { return sqrtf(v.x * v.x + v.y * v.y); }
void vector_Normalization(Vector *v) // 归一化
{
    float norm = vector_norm(*v);
    if (norm != 0)
    {
        v->x /= norm;
        v->y /= norm;
    }
}
// 获取从p1指向p2的向量
Vector vector_get(float x1, float y1, float x2, float y2) { return (Vector){x2 - x1, y2 - y1}; }

// 获取两个向量的点积
float vector_dot(Vector v1, Vector v2) { return v1.x * v2.x + v1.y * v2.y; }

// 获取A与B的叉积
float vector_cross(Vector v1, Vector v2) { return v1.x * v2.y - v1.y * v2.x; }

// 获取反向向量
Vector negate_vector(Vector v) { return (Vector){-v.x, -v.y}; }
// 逆时针转90度
Vector counterclockwise_90(Vector v) { return (Vector){-v.y, v.x}; }
// 顺时针转90度
Vector clockwise_90(Vector v) { return (Vector){v.y, -v.x}; }

// 逆时针转45度
Vector counterclockwise_45(Vector v) { return (Vector){COS45 * (v.x - v.y), COS45 * (v.x + v.y)}; }

Vector clockwise_45(Vector v) { return (Vector){COS45 * (v.x + v.y), COS45 * (v.y - v.x)}; }
Vector counterclockwise(Vector v, float angle)
{
    float cosB = cosf(Angle_To_Rad(angle));
    float sinB = sinf(Angle_To_Rad(angle));
    return (Vector){v.x * cosB - v.y * sinB, v.y * cosB + v.x * sinB};
}
SDL_FPoint Get_FPoint_From_parametric_equation(const SDL_FPoint p0, Vector direction, float radius)
{
    vector_Normalization(&direction);
    return (SDL_FPoint){p0.x + direction.x * radius, p0.y + direction.y * radius};
}

// 角度转弧度
inline float Angle_To_Rad(float angle) { return angle * 0.0174533; }