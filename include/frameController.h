#ifndef FRAME_CONTROLLER_H
#define FRAME_CONTROLLER_H
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <stdbool.h>

typedef struct
{
    Uint64 lastLogic; // 上次逻辑更新时间
    // Uint64 lastRender;      // 上次渲染时间
    Uint64 current;     // 当前时间
    double accumulator; // 时间积累器
    double logicAimDt;  // 逻辑更新目标时间间隔（秒）
    int logicCount;     // 逻辑更新计数
    int renderCount;    // 渲染帧计数
    Uint64 lastFPS;     // 上次FPS更新时间(用于更新FPS显示)
} FrameController;

// 初始化帧率控制器
void FrameController_Init(FrameController *fc, int logicRate);

// 更新时间并检查是否需要逻辑更新
int FrameController_Update(FrameController *fc);

// 增加渲染计数(每次更新完调用)
void FrameController_AddRenderCount(FrameController *fc);

// 更新FPS
void FrameController_UpdateFPS(FrameController *fc, float *fps, float *ups);

#endif