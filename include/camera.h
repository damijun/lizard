#ifndef CAMERA_H
#define CAMERA_H
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdbool.h>

#define DEFALUT_MIN_ZOOM 0.5f
#define DEFALUT_MAX_ZOOM 2.0f
#define DEFALUT_MAX_CAMERA_X 10000.0f
#define DEFALUT_MAX_CAMERA_Y 10000.0f
typedef struct
{
    // 相机位置（世界坐标）
    float x, y;

    // 相机缩放级别（1.0为原始大小，大于1.0为放大，小于1.0为缩小）
    float zoom;
    // 缩放范围限制
    float minZoom, maxZoom;

    // 相机边界（可选，限制相机移动范围）
    float minX, maxX, minY, maxY;
    // 相机是否启用边界限制
    bool useBounds;

    // 屏幕尺寸（用于坐标转换）
    int screenWidth, screenHeight;
} Camera;

// 初始化相机
Camera *Camera_Create(float x, float y, int screenWidth, int screenHeight);

// 销毁相机
void Camera_Destroy(Camera *camera);

// 设置屏幕尺寸（窗口大小改变时调用）
void Camera_SetScreenSize(Camera *camera, int width, int height);

// 设置相机边界
void Camera_SetBounds(Camera *camera, float minX, float maxX, float minY, float maxY);

// 世界坐标转换为屏幕坐标（考虑相机位置和缩放）
SDL_FPoint Camera_WorldToScreen(const Camera *camera, float worldX, float worldY);

// 屏幕坐标转换为世界坐标（考虑相机位置和缩放）
SDL_FPoint Camera_ScreenToWorld(const Camera *camera, float screenX, float screenY);

// 获取鼠标在世界坐标系中的位置
SDL_FPoint Camera_GetMouseWorldPosition(const Camera *camera, SDL_Renderer *renderer);

// 设置相机位置
void Camera_SetPosition(Camera *camera, float x, float y);

// 移动相机
void Camera_Move(Camera *camera, float dx, float dy);

// 缩放相机
void Camera_Zoom_Add(Camera *camera, float zoomDelta);

// 重置相机
void Camera_Reset(Camera *camera);

// 获取相机在世界坐标系中的视图矩形（可见区域）左上角坐标,加向右的宽度和向下的高度
SDL_FRect Camera_GetViewRect(const Camera *camera);

#endif