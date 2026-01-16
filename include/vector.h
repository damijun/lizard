#ifndef VECTOR_H
#define VECTOR_H
#define COS45 0.707107f
#include <SDL3/SDL.h>
typedef struct
{
    float x;
    float y;
} Vector;
float vector_norm(Vector v);                               // 获取向量的模
void vector_Normalization(Vector *v);                      // 归一化
Vector vector_get(float x1, float y1, float x2, float y2); // 获取p1指向p2的向量
float vector_dot(Vector v1, Vector v2);                    // 获取两个向量的点积
float vector_cross(Vector v1, Vector v2);                  // 获取A与B的叉积
Vector negate_vector(Vector v);                            // 获取反向向量
Vector counterclockwise_90(Vector v);                      // 逆时针转90度
Vector clockwise_90(Vector v);                             // 顺时针转90度
Vector counterclockwise_45(Vector v);                      // 逆时针转45度
Vector clockwise_45(Vector v);                             // 顺时针转45度
Vector counterclockwise(Vector v, float angle);            // 逆时针转angle度
SDL_FPoint Get_FPoint_From_parametric_equation(const SDL_FPoint p0, Vector direction, float radius);
float Angle_To_Rad(float angle); // 角度转弧度
#endif