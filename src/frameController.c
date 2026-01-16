#include "frameController.h"

// 初始化帧率控制器
void FrameController_Init(FrameController *fc, int logicRate)
{
    fc->lastLogic = SDL_GetPerformanceCounter();
    // fc->lastRender = SDL_GetPerformanceCounter();
    fc->current = SDL_GetPerformanceCounter();
    fc->logicAimDt = 1 / (double)logicRate;
    fc->accumulator = 0.0;
    fc->logicCount = 0;
    fc->renderCount = 0;
    fc->lastFPS = SDL_GetPerformanceCounter();
}

// 更新时间并检查是否需要逻辑更新
int FrameController_Update(FrameController *fc)
{
    fc->current = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    double dt = (double)(fc->current - fc->lastLogic) / (double)frequency;
    fc->lastLogic = fc->current;
    fc->accumulator += dt;
    int updateNeeded = 0;
    while (fc->accumulator >= fc->logicAimDt)
    {
        fc->accumulator -= fc->logicAimDt;
        fc->logicCount++;
        updateNeeded++;
    }
    return updateNeeded;
}

// 增加渲染计数(每次更新完调用)
void FrameController_AddRenderCount(FrameController *fc) { fc->renderCount++; }

// 更新FPS
void FrameController_UpdateFPS(FrameController *fc, float *fps, float *ups)
{
    fc->current = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float dt = (float)(fc->current - fc->lastFPS) / (float)frequency;
    if (dt >= 1.0f)
    {
        *ups = fc->logicCount / dt;
        *fps = fc->renderCount / dt;
        fc->lastFPS = fc->current;
        fc->logicCount = 0;
        fc->renderCount = 0;
    }
}
