#include "camera.h"
// 初始化相机
// 初始化相机
Camera *Camera_Create(float x, float y, int screenWidth, int screenHeight)
{
    Camera *camera = (Camera *)SDL_malloc(sizeof(Camera));
    if (!camera) return NULL;

    // 设置初始位置
    camera->x = x;
    camera->y = y;

    // 设置默认缩放
    camera->zoom = 1.0f;
    camera->minZoom = DEFALUT_MIN_ZOOM;
    camera->maxZoom = DEFALUT_MAX_ZOOM;

    // 设置屏幕尺寸
    camera->screenWidth = screenWidth;
    camera->screenHeight = screenHeight;

    // 初始化边界
    camera->minX = -DEFALUT_MAX_CAMERA_X;
    camera->maxX = DEFALUT_MAX_CAMERA_X;
    camera->minY = -DEFALUT_MAX_CAMERA_Y;
    camera->maxY = DEFALUT_MAX_CAMERA_Y;
    camera->useBounds = false;

    return camera;
}

// 销毁相机
void Camera_Destroy(Camera *camera)
{
    if (camera)
    {
        SDL_free(camera);
    }
}

// 设置屏幕尺寸
void Camera_SetScreenSize(Camera *camera, int width, int height)
{
    if (!camera) return;

    camera->screenWidth = width;
    camera->screenHeight = height;
}

// 设置相机边界
void Camera_SetBounds(Camera *camera, float minX, float maxX, float minY, float maxY)
{
    if (!camera) return;

    camera->minX = minX;
    camera->maxX = maxX;
    camera->minY = minY;
    camera->maxY = maxY;
    camera->useBounds = true;

    // 立即应用边界限制
    Camera_SetPosition(camera, camera->x, camera->y);
}

// 世界坐标转换为屏幕坐标
SDL_FPoint Camera_WorldToScreen(const Camera *camera, float worldX, float worldY)
{
    SDL_FPoint screenPoint = {0, 0};

    if (!camera) return screenPoint;

    /*
    原理：
    1. 屏幕坐标系：原点在左上角，向右为x正方向，向下为y正方向
    2. 世界坐标系：笛卡尔坐标系，向右为x正方向，向上为y正方向

    转换步骤：
    1. 将世界坐标转换为相对于相机的坐标
    2. 应用缩放
    3. 将坐标平移到屏幕中心（相机在屏幕中心）
    4. 翻转y轴（因为屏幕y轴向下为正，世界y轴向上为正）

    公式：
    screenX = (worldX - cameraX) * zoom + screenWidth/2
    screenY = (cameraY - worldY) * zoom + screenHeight/2

    解释：
    - (worldX - cameraX): 世界坐标相对于相机的位置
    - * zoom: 应用缩放
    - + screenWidth/2: 将相机位置平移到屏幕中心
    - cameraY - worldY: 因为世界y轴向上为正，所以需要反转
    */

    screenPoint.x = (worldX - camera->x) * camera->zoom + camera->screenWidth / 2.0f;
    screenPoint.y = (camera->y - worldY) * camera->zoom + camera->screenHeight / 2.0f;

    return screenPoint;
}

// 屏幕坐标转换为世界坐标
SDL_FPoint Camera_ScreenToWorld(const Camera *camera, float screenX, float screenY)
{
    SDL_FPoint worldPoint = {0, 0};

    if (!camera || camera->zoom == 0) return worldPoint;

    /*
    原理：
    世界坐标转换的逆运算

    公式：
    worldX = (screenX - screenWidth/2) / zoom + cameraX
    worldY = cameraY - (screenY - screenHeight/2) / zoom
    */

    worldPoint.x = (screenX - camera->screenWidth / 2.0f) / camera->zoom + camera->x;
    worldPoint.y = camera->y - (screenY - camera->screenHeight / 2.0f) / camera->zoom;

    return worldPoint;
}

// 获取鼠标在世界坐标系中的位置
SDL_FPoint Camera_GetMouseWorldPosition(const Camera *camera, SDL_Renderer *renderer)
{
    SDL_FPoint worldPoint = {0.0f, 0.0f};

    // 参数检查
    if (!camera || !renderer)
    {
        SDL_Log("错误:Camera_GetMouseWorldPosition 参数为空\n");
        return worldPoint;
    }

    // 步骤1：获取鼠标在窗口中的坐标
    float windowMouseX, windowMouseY;
    float mouseButtons = SDL_GetMouseState(&windowMouseX, &windowMouseY);

    // 步骤2：将窗口坐标转换为逻辑坐标
    // 这是关键步骤！因为SDL_SetRenderLogicalPresentation会影响坐标映射
    float logicalMouseX, logicalMouseY;
    SDL_RenderCoordinatesFromWindow(renderer, windowMouseX, windowMouseY, &logicalMouseX, &logicalMouseY);

    // 步骤3：将逻辑坐标转换为世界坐标
    worldPoint = Camera_ScreenToWorld(camera, logicalMouseX, logicalMouseY);

    return worldPoint;
}

// 设置相机位置
void Camera_SetPosition(Camera *camera, float x, float y)
{
    if (!camera) return;

    // 应用边界限制
    if (camera->useBounds)
    {
        camera->x = SDL_clamp(x, camera->minX, camera->maxX);
        camera->y = SDL_clamp(y, camera->minY, camera->maxY);
    }
    else
    {
        camera->x = x;
        camera->y = y;
    }
}

// 移动相机
void Camera_Move(Camera *camera, float dx, float dy)
{
    if (!camera) return;

    /*
    移动相机时，dx和dy是世界坐标中的移动距离
    注意：因为世界y轴向上为正，所以dy为正表示向上移动
    */
    Camera_SetPosition(camera, camera->x + dx, camera->y + dy);
}

// 缩放相机（以屏幕中心/相机位置为中心缩放）
void Camera_Zoom_Add(Camera *camera, float zoomAdd)
{
    if (!camera) return;

    /*
    原理：以屏幕中心（相机位置）为中心缩放

    关键点：
    1. 屏幕中心点对应的是相机位置 (cameraX, cameraY)
    2. 当我们改变缩放时，我们希望屏幕中心仍然显示同一个世界坐标
    3. 因为屏幕中心就是相机位置，所以缩放时相机位置不需要调整

    注意：这里我们只改变缩放值，不调整相机位置
    因为以屏幕中心为中心缩放时，屏幕中心对应的世界坐标就是相机位置
    缩放后，屏幕中心仍然显示相机位置，所以相机位置不需要改变
    */

    // 计算出新的缩放值
    float newZoom = camera->zoom + zoomAdd;

    // 限制缩放范围
    newZoom = SDL_clamp(newZoom, camera->minZoom, camera->maxZoom);

    // 更新缩放值
    camera->zoom = newZoom;
}

void Camera_Reset(Camera *camera)
{
    if (!camera) return;

    camera->x = 0.0f;
    camera->y = 0.0f;
    camera->zoom = 1.0f;
}

// camera.c 中的 Camera_GetViewRect 函数
SDL_FRect Camera_GetViewRect(const Camera *camera)
{
    SDL_FRect viewRect = {0, 0, 0, 0};

    if (!camera) return viewRect;

    /*
    计算相机可见的世界区域
    屏幕四个角对应的世界坐标就是可见区域的边界

    注意：由于我们的坐标系中y轴向上为正，所以：
    - topLeft: 屏幕左上角对应的世界坐标
    - topRight: 屏幕右上角对应的世界坐标
    - bottomLeft: 屏幕左下角对应的世界坐标
    - bottomRight: 屏幕右下角对应的世界坐标

    但是要注意：由于y轴向上为正，屏幕左下角的y值可能比右上角的y值小
    所以我们需要找出最小和最大的x、y值
    */

    // 屏幕四个角
    SDL_FPoint topLeft = Camera_ScreenToWorld(camera, 0, 0);
    SDL_FPoint bottomRight = Camera_ScreenToWorld(camera, camera->screenWidth, camera->screenHeight);

    float minX = topLeft.x;
    float maxX = bottomRight.x;
    float minY = bottomRight.y;
    float maxY = topLeft.y;
    viewRect.x = minX;
    viewRect.y = maxY;
    viewRect.w = maxX - minX;
    viewRect.h = maxY - minY;

    return viewRect;
}