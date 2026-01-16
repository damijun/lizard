#include "character.h"
#include "polygon.h"
#include "vector.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// 渲染纹理,带目标位置(屏幕位置),旋转中心和旋转角度(角度制逆时针),和缩放系数,旋转中心默认为纹理中心
void Render_Texture(SDL_Renderer *renderer, SDL_Texture *texture, SDL_FPoint pos, float angle, float scale)
{
    float texWidth, texHeight;
    SDL_GetTextureSize(texture, &texWidth, &texHeight);
    texWidth *= scale;
    texHeight *= scale;
    SDL_FRect dst = {pos.x - texWidth / 2, pos.y - texHeight / 2, texWidth, texHeight};
    SDL_RenderTextureRotated(renderer, texture, NULL, &dst, -angle, NULL, SDL_FLIP_NONE);
}

// 两个包围盒是否发生碰撞
bool AABBBoxCollision(AABBBox a, AABBBox b) { return !(a.minY > b.maxY || a.minX > b.maxX || b.minY > a.maxY || b.minX > a.maxX); }

AABBBox Rect_To_AABBBox(SDL_FRect rect) { return (AABBBox){rect.x, rect.x + rect.w, rect.y - rect.h, rect.y}; }
SDL_FRect AABBBox_To_Rect(AABBBox box) { return (SDL_FRect){box.minX, box.minY, box.maxX - box.minX, box.maxY - box.minY}; }

// 更新角色的AABB包围盒
void Character_UpdateAABBBox(Character *character)
{
    float minX = character->body[0].x - character->body[0].radius * 2;
    float maxX = character->body[0].x + character->body[0].radius * 2;
    float minY = character->body[0].y - character->body[0].radius * 2;
    float maxY = character->body[0].y + character->body[0].radius * 2;
    for (int i = 0; i < character->bodyCount; i++)
    {
        node *n = &character->body[i];
        float nmaxX = n->x + n->radius;
        float nmaxY = n->y + n->radius;
        float nminX = n->x - n->radius;
        float nminY = n->y - n->radius;
        if (nminX < minX)
        {
            minX = nminX;
        }
        if (nmaxX > maxX)
        {
            maxX = nmaxX;
        }
        if (nminY < minY)
        {
            minY = nminY;
        }
        if (nmaxY > maxY)
        {
            maxY = nmaxY;
        }
    }
    character->box.minX = minX;
    character->box.maxX = maxX;
    character->box.minY = minY;
    character->box.maxY = maxY;
}

// 创建并初始化角色
Character *Character_Creat(enum CharacterType type, float x, float y, Vector initialDirection, float initialSpeed, const float *radiusList, const float *distanceList, const float *flexibility, const int bodyCount, SDL_FColor color, SDL_FColor outLineColor, const Chain3 legs[2])
{
    if (!radiusList || !distanceList || bodyCount <= 0)
    {
        return NULL;
    }
    Character *character = (Character *)malloc(sizeof(Character));
    if (!character) return NULL;

    // 赋值类型
    character->type = type;
    if (type == LIZARD)
    {
        if (!legs)
        {
            free(character);
            return NULL;
        }
        character->legs[0] = legs[0];
        character->legs[1] = legs[0];
        character->legs[2] = legs[1];
        character->legs[3] = legs[1];
        character->frontLegPos = 2;
        character->backLegPos = 6;
        character->frontLegMaxDistance = 70.0f;
        character->backLegMaxDistance = 50.0f;
        character->legOutDistance = 55.0f; // 向外伸展
        character->legOutDistanceBack = 65.0f;
        character->forwardDis[0] = 10.0f; // 向前伸展
        character->forwardDis[1] = 5.0f;
    }

    // 分配节点
    character->body = (node *)malloc(bodyCount * sizeof(node));
    if (!character->body)
    {
        free(character);
        return NULL;
    }
    vector_Normalization(&initialDirection);
    for (int i = 0; i < bodyCount; i++)
    {
        character->body[i].radius = radiusList[i];
        character->body[i].distance = distanceList[i];
        character->body[i].flexibility = flexibility[i];
        if (i == 0)
        {
            character->body[i].x = x;
            character->body[i].y = y;
        }
        else
        {
            character->body[i].x = character->body[i - 1].x - distanceList[i - 1] * initialDirection.x;
            character->body[i].y = character->body[i - 1].y - distanceList[i - 1] * initialDirection.y;
        }
    }

    // 初始化成员
    character->bodyCount = bodyCount;

    character->headInside = 1.0f;
    character->head30Inside = 1.0f;
    character->head60Inside = 1.0f;
    character->direction = initialDirection;
    character->turnSpeed = DEFALUT_MAX_TURN_SPEED;
    character->speed = initialSpeed;
    character->maxSpeed = DEFALUT_MAX_SPEED;
    character->minSpeed = DEFALUT_MIN_SPEED;

    // character->renderBox = (AABBBox){0.0f, 0.0f, 100.0f, 100.0f};
    character->needRender = false;
    character->eyeInside = 0.8f;
    character->eyeRadius = 5.0f;
    character->eyeColor = (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f};
    character->color = color;
    character->outLineColor = outLineColor;

    // 状态部分
    character->HP = character->maxHP = 100.0f;
    character->haveHeadGun = false;
    character->haveTailGun = false;
    printf("初始化完成\n");
    return character;
}

// 销毁角色
void Character_Destroy(Character *character)
{
    if (!character) return;
    if (character->body) free(character->body);
    free(character);
}

// 更新是否需要渲染
void Character_check_render(Character *character, const Camera *camera) { character->needRender = AABBBoxCollision(character->renderBox, Rect_To_AABBBox(Camera_GetViewRect(camera))); }

void AddPointToOutline(SDL_FPoint *points, int *count, SDL_FPoint p)
{
    points[*count] = p;
    (*count)++;
}
void Character_render(const Character *character, SDL_Renderer *renderer, const Camera *camera, BatchRenderer *batch)
{
    if (!character->needRender || !character || !renderer || !camera || !batch)
    {
        return;
    }
    // 判断如果type是蜥蜴,就提前画腿
    if (character->type == LIZARD)
    {
        for (int i = 0; i < 4; i++)
        {
            Chain3 leg = character->legs[i];
            Polygon_DrawCircle(batch, leg.head.x, leg.head.y, leg.headRadius, 10, character->color, camera);
            Vector rootToMiddle = vector_get(leg.root.x, leg.root.y, leg.middle.x, leg.middle.y);
            Vector middleToHead = vector_get(leg.middle.x, leg.middle.y, leg.head.x, leg.head.y);
            Vector rootDraw = counterclockwise_90(rootToMiddle);
            Vector headDraw = counterclockwise_90(middleToHead);
            Vector middleDraw = (Vector){-rootToMiddle.x + middleToHead.x, -rootToMiddle.y + middleToHead.y};
            SDL_FPoint root1 = Get_FPoint_From_parametric_equation(leg.root, rootDraw, leg.rootRadius);
            SDL_FPoint root2 = Get_FPoint_From_parametric_equation(leg.root, negate_vector(rootDraw), leg.rootRadius);
            SDL_FPoint middle1;
            SDL_FPoint middle2;
            if (i == 1 || i == 2)
            {
                middle1 = Get_FPoint_From_parametric_equation(leg.middle, middleDraw, leg.middleRadius);
                middle2 = Get_FPoint_From_parametric_equation(leg.middle, negate_vector(middleDraw), leg.middleRadius);
            }
            else
            {
                middle1 = Get_FPoint_From_parametric_equation(leg.middle, negate_vector(middleDraw), leg.middleRadius);
                middle2 = Get_FPoint_From_parametric_equation(leg.middle, middleDraw, leg.middleRadius);
            }
            SDL_FPoint head1 = Get_FPoint_From_parametric_equation(leg.head, headDraw, leg.headRadius);
            SDL_FPoint head2 = Get_FPoint_From_parametric_equation(leg.head, negate_vector(headDraw), leg.headRadius);

            // 画填充
            Polygon_DrawTriangle(batch, leg.root.x, leg.root.y, root1.x, root1.y, middle1.x, middle1.y, character->color, camera);
            Polygon_DrawTriangle(batch, leg.root.x, leg.root.y, root2.x, root2.y, middle2.x, middle2.y, character->color, camera);
            // Polygon_DrawTriangle(batch, leg.root.x, leg.root.y, root1.x, root1.y, middle2.x, middle2.y, character->color, camera);
            // Polygon_DrawTriangle(batch, leg.root.x, leg.root.y, root2.x, root2.y, middle1.x, middle1.y, character->color, camera);
            Polygon_DrawTriangle(batch, leg.root.x, leg.root.y, leg.middle.x, leg.middle.y, middle1.x, middle1.y, character->color, camera);
            Polygon_DrawTriangle(batch, leg.root.x, leg.root.y, leg.middle.x, leg.middle.y, middle2.x, middle2.y, character->color, camera);
            Polygon_DrawTriangle(batch, leg.head.x, leg.head.y, head1.x, head1.y, middle1.x, middle1.y, character->color, camera);
            Polygon_DrawTriangle(batch, leg.head.x, leg.head.y, head2.x, head2.y, middle2.x, middle2.y, character->color, camera);
            // Polygon_DrawTriangle(batch, leg.head.x, leg.head.y, head1.x, head1.y, middle2.x, middle2.y, character->color, camera);
            // Polygon_DrawTriangle(batch, leg.head.x, leg.head.y, head2.x, head2.y, middle1.x, middle1.y, character->color, camera);
            Polygon_DrawTriangle(batch, leg.head.x, leg.head.y, leg.middle.x, leg.middle.y, middle1.x, middle1.y, character->color, camera);
            Polygon_DrawTriangle(batch, leg.head.x, leg.head.y, leg.middle.x, leg.middle.y, middle2.x, middle2.y, character->color, camera);

            // 画描边
            SDL_FPoint top = Get_FPoint_From_parametric_equation(leg.head, middleToHead, leg.headRadius);
            SDL_FPoint left45 = Get_FPoint_From_parametric_equation(leg.head, counterclockwise_45(middleToHead), leg.headRadius);
            SDL_FPoint right45 = Get_FPoint_From_parametric_equation(leg.head, clockwise_45(middleToHead), leg.headRadius);
            SDL_FPoint left = Get_FPoint_From_parametric_equation(leg.head, counterclockwise_90(middleToHead), leg.headRadius);
            SDL_FPoint right = Get_FPoint_From_parametric_equation(leg.head, clockwise_90(middleToHead), leg.headRadius);
            SDL_FPoint legOutline[9] = {root1, middle1, left, left45, top, right45, right, middle2, root2};

            Polygon_DrawLines(batch, legOutline, 9, DEFALUT_OUTLINE_WIDTH, character->outLineColor, camera);
        }
    }

    // 关于描边,用画两个三角形表示一条粗线段,总顶点数量为:4*(节点数量+1)
    // 申请一块内存空间存放顶点位置,最后统一提交给批批渲染器
    int pointCountL = 0;
    SDL_FPoint *outlinePointsL = (SDL_FPoint *)malloc(sizeof(SDL_FPoint) * (2 * character->bodyCount + 4)); // 左边的描边
    int pointCountR = 0;
    SDL_FPoint *outlinePointsR = (SDL_FPoint *)malloc(sizeof(SDL_FPoint) * (2 * character->bodyCount + 4)); // 右边的描边

    // 头部取点(特殊处理)
    node *head = &character->body[0];
    SDL_FPoint headPoint = (SDL_FPoint){head->x, head->y};
    // 计算头部渲染方向
    Vector renderDirection = vector_get(character->body[1].x, character->body[1].y, headPoint.x, headPoint.y); // 渲染头部使用的方向向量,由头部后面一两个节点计算出来
    // 填充
    SDL_FPoint headTop = Get_FPoint_From_parametric_equation(headPoint, renderDirection, head->radius * character->headInside);
    SDL_FPoint headLeft30 = Get_FPoint_From_parametric_equation(headPoint, counterclockwise(renderDirection, 30), head->radius * character->head30Inside);
    SDL_FPoint headRight30 = Get_FPoint_From_parametric_equation(headPoint, counterclockwise(renderDirection, -30), head->radius * character->head30Inside);
    SDL_FPoint headLeft60 = Get_FPoint_From_parametric_equation(headPoint, counterclockwise(renderDirection, 60), head->radius * character->head60Inside);
    SDL_FPoint headRight60 = Get_FPoint_From_parametric_equation(headPoint, counterclockwise(renderDirection, -60), head->radius * character->head60Inside);
    SDL_FPoint headLeft = Get_FPoint_From_parametric_equation(headPoint, counterclockwise_90(renderDirection), head->radius);
    SDL_FPoint headRight = Get_FPoint_From_parametric_equation(headPoint, clockwise_90(renderDirection), head->radius);
    Polygon_DrawTriangle(batch, headPoint.x, headPoint.y, headTop.x, headTop.y, headLeft30.x, headLeft30.y, character->color, camera);
    Polygon_DrawTriangle(batch, headPoint.x, headPoint.y, headTop.x, headTop.y, headRight30.x, headRight30.y, character->color, camera);
    Polygon_DrawTriangle(batch, headPoint.x, headPoint.y, headLeft30.x, headLeft30.y, headLeft60.x, headLeft60.y, character->color, camera);
    Polygon_DrawTriangle(batch, headPoint.x, headPoint.y, headRight30.x, headRight30.y, headRight60.x, headRight60.y, character->color, camera);
    Polygon_DrawTriangle(batch, headPoint.x, headPoint.y, headLeft60.x, headLeft60.y, headLeft.x, headLeft.y, character->color, camera);
    Polygon_DrawTriangle(batch, headPoint.x, headPoint.y, headRight60.x, headRight60.y, headRight.x, headRight.y, character->color, camera);
    // 描边左
    AddPointToOutline(outlinePointsL, &pointCountL, headRight30); // 额外添加右30度点,以获得描边平滑
    AddPointToOutline(outlinePointsL, &pointCountL, headTop);
    AddPointToOutline(outlinePointsL, &pointCountL, headLeft30);
    AddPointToOutline(outlinePointsL, &pointCountL, headLeft60);
    AddPointToOutline(outlinePointsL, &pointCountL, headLeft);

    // 描边右
    AddPointToOutline(outlinePointsR, &pointCountR, headTop);
    AddPointToOutline(outlinePointsR, &pointCountR, headRight30);
    AddPointToOutline(outlinePointsR, &pointCountR, headRight60);
    AddPointToOutline(outlinePointsR, &pointCountR, headRight);

    // 初始化上一个身体节点的左右点
    SDL_FPoint lastBodyLeft = headLeft;
    SDL_FPoint lastBodyRight = headRight;

    // 初始化lastLeft2和lastRight2为头部连接点
    SDL_FPoint lastLeft2 = headLeft;
    SDL_FPoint lastRight2 = headRight;

    // printf("开始身体绘制，节点数: %d\n", character->bodyCount);

    // 遍历身体节点（从第二个节点开始）
    for (int i = 1; i < character->bodyCount; i++)
    {
        node *lastBody = &character->body[i - 1]; // 仅用于计算方向
        node *nowBody = &character->body[i];

        // 计算当前身体的方向
        Vector nowBodyDirection = vector_get(nowBody->x, nowBody->y, lastBody->x, lastBody->y);

        // 计算当前节点的左右边界点
        SDL_FPoint nowBodyPoint = (SDL_FPoint){nowBody->x, nowBody->y};
        SDL_FPoint nowBodyLeft = Get_FPoint_From_parametric_equation(nowBodyPoint, counterclockwise_90(nowBodyDirection), nowBody->radius);
        SDL_FPoint nowBodyRight = Get_FPoint_From_parametric_equation(nowBodyPoint, clockwise_90(nowBodyDirection), nowBody->radius);

        // 计算平滑点（使用切割距离平滑连接）
        SDL_FPoint left1 = Get_far_point(nowBodyLeft, lastBodyLeft, CUTTING_DISTANCE);
        SDL_FPoint right1 = Get_far_point(nowBodyRight, lastBodyRight, CUTTING_DISTANCE);
        SDL_FPoint left2 = Get_far_point(lastBodyLeft, nowBodyLeft, CUTTING_DISTANCE);
        SDL_FPoint right2 = Get_far_point(lastBodyRight, nowBodyRight, CUTTING_DISTANCE);

        // 用平滑点绘制连接两个身体节点的四边形（用两个三角形表示）
        Polygon_DrawTriangle(batch, lastLeft2.x, lastLeft2.y, lastRight2.x, lastRight2.y, left1.x, left1.y, character->color, camera);
        Polygon_DrawTriangle(batch, left1.x, left1.y, right1.x, right1.y, lastRight2.x, lastRight2.y, character->color, camera);
        // 用平滑点绘制身体内部的四边形( 用两个三角形表示)
        Polygon_DrawTriangle(batch, left1.x, left1.y, right1.x, right1.y, left2.x, left2.y, character->color, camera);
        Polygon_DrawTriangle(batch, left2.x, left2.y, right2.x, right2.y, right1.x, right1.y, character->color, camera);
        // 描边左
        if (i != 1)
        {
            AddPointToOutline(outlinePointsL, &pointCountL, left1);
        }
        if (i != character->bodyCount - 1)
        {
            AddPointToOutline(outlinePointsL, &pointCountL, left2);
        }

        // 描边右
        if (i != 1)
        {
            AddPointToOutline(outlinePointsR, &pointCountR, right1);
        }
        if (i != character->bodyCount - 1)
        {
            AddPointToOutline(outlinePointsR, &pointCountR, right2);
        }

        // 更新上一节点的信息
        lastLeft2 = left2;
        lastRight2 = right2;
        lastBodyLeft = nowBodyLeft;
        lastBodyRight = nowBodyRight;
    }

    // 处理尾部（需要至少2个节点）
    // 现在lastBodyleft就是尾部节点的左边点
    // 只需再取尾部顶点,顶偏左45度点,顶偏右45度点
    node *tail = &character->body[character->bodyCount - 1];
    node *nodeBeforeTail = &character->body[character->bodyCount - 2];
    Vector tailDirection = vector_get(nodeBeforeTail->x, nodeBeforeTail->y, tail->x, tail->y);
    SDL_FPoint tailPoint = (SDL_FPoint){tail->x, tail->y};
    SDL_FPoint tailTop = Get_FPoint_From_parametric_equation(tailPoint, tailDirection, tail->radius);
    SDL_FPoint tailLeft45 = Get_FPoint_From_parametric_equation(tailPoint, counterclockwise_45(tailDirection), tail->radius);
    SDL_FPoint tailRight45 = Get_FPoint_From_parametric_equation(tailPoint, clockwise_45(tailDirection), tail->radius);
    // 渲染身体和尾巴连接处
    Polygon_DrawTriangle(batch, lastLeft2.x, lastLeft2.y, lastRight2.x, lastRight2.y, lastBodyLeft.x, lastBodyLeft.y, character->color, camera);
    Polygon_DrawTriangle(batch, lastRight2.x, lastRight2.y, lastBodyLeft.x, lastBodyLeft.y, lastBodyRight.x, lastBodyRight.y, character->color, camera);
    // 渲染尾巴的四个三角形
    Polygon_DrawTriangle(batch, tailPoint.x, tailPoint.y, tailTop.x, tailTop.y, tailLeft45.x, tailLeft45.y, character->color, camera);
    Polygon_DrawTriangle(batch, tailPoint.x, tailPoint.y, tailTop.x, tailTop.y, tailRight45.x, tailRight45.y, character->color, camera);
    Polygon_DrawTriangle(batch, tailPoint.x, tailPoint.y, tailLeft45.x, tailLeft45.y, lastBodyRight.x, lastBodyRight.y, character->color, camera);
    Polygon_DrawTriangle(batch, tailPoint.x, tailPoint.y, tailRight45.x, tailRight45.y, lastBodyLeft.x, lastBodyLeft.y, character->color, camera);
    // 描边左
    AddPointToOutline(outlinePointsL, &pointCountL, lastBodyLeft);
    AddPointToOutline(outlinePointsL, &pointCountL, tailRight45);
    AddPointToOutline(outlinePointsL, &pointCountL, tailTop);
    // 描边右
    AddPointToOutline(outlinePointsR, &pointCountR, lastBodyRight);
    AddPointToOutline(outlinePointsR, &pointCountR, tailLeft45);
    AddPointToOutline(outlinePointsR, &pointCountR, tailTop);
    AddPointToOutline(outlinePointsR, &pointCountR, tailRight45); // 额外添加右边45度的点以获得描边平滑

    // 渲染描边
    Polygon_DrawLines(batch, outlinePointsL, pointCountL, DEFALUT_OUTLINE_WIDTH, character->outLineColor, camera);
    Polygon_DrawLines(batch, outlinePointsR, pointCountR, DEFALUT_OUTLINE_WIDTH, character->outLineColor, camera);
    // 释放描边顶点数组的内存
    SDL_free(outlinePointsL);
    SDL_free(outlinePointsR);

    // 渲染眼睛
    SDL_FPoint leftEye = Get_FPoint_From_parametric_equation(headPoint, counterclockwise_90(renderDirection), head->radius * character->eyeInside);
    SDL_FPoint rightEye = Get_FPoint_From_parametric_equation(headPoint, clockwise_90(renderDirection), head->radius * character->eyeInside);
    Polygon_DrawCircle(batch, leftEye.x, leftEye.y, character->eyeRadius, 8, character->eyeColor, camera);
    Polygon_DrawCircle(batch, rightEye.x, rightEye.y, character->eyeRadius, 8, character->eyeColor, camera);

    // 渲染包围盒(for debug)
    /*
    Polygon_DrawLine(batch, character->box.minX, character->box.minY, character->box.maxX, character->box.minY, DEFALUT_OUTLINE_WIDTH, character->color, camera);
    Polygon_DrawLine(batch, character->box.minX, character->box.minY, character->box.minX, character->box.maxY, DEFALUT_OUTLINE_WIDTH, character->color, camera);
    Polygon_DrawLine(batch, character->box.minX, character->box.maxY, character->box.maxX, character->box.maxY, DEFALUT_OUTLINE_WIDTH, character->color, camera);
    Polygon_DrawLine(batch, character->box.maxX, character->box.minY, character->box.maxX, character->box.maxY, DEFALUT_OUTLINE_WIDTH, character->color, camera);
    */
}

// 渲染枪
void Gun_render(const Gun *gun, Vector direction, SDL_Renderer *renderer, const Camera *camera, SDL_Texture *textureList[3])
{
    float angle = 57.3 * atan2f(direction.y, direction.x);
    Render_Texture(renderer, textureList[gun->type], Camera_WorldToScreen(camera, gun->x, gun->y), angle - 90, 5 * camera->zoom);
}
// 获取角色头节点指向鼠标的方向(不进行归一化处理)
Vector Character_get_head_to_mouse_direction(const Character *character, SDL_Renderer *renderer, const Camera *camera)
{
    SDL_FPoint mouseWorld = Camera_GetMouseWorldPosition(camera, renderer);
    return vector_get(character->body[0].x, character->body[0].y, mouseWorld.x, mouseWorld.y);
}

void Character_directly_turn_to_mouse(Character *character, SDL_Renderer *renderer, const Camera *camera) // 角色直接面向鼠标方向(for debug)
{
    Vector headToMouse = Character_get_head_to_mouse_direction(character, renderer, camera);
    vector_Normalization(&headToMouse);
    character->direction = headToMouse;
}
void Character_turn_to_vector(Character *character, Vector direction)
{
    float cosAngle = vector_dot(character->direction, direction) / (vector_norm(character->direction) * vector_norm(direction));
    float cosTurnSpeed = cosf(Angle_To_Rad(character->turnSpeed));
    if (cosAngle > 0 && cosAngle > cosTurnSpeed)
    {
        vector_Normalization(&direction);
        character->direction = direction;
    }
    else
    {
        float cross = vector_cross(character->direction, direction);
        if (cross > 0)
        {
            character->direction = counterclockwise(character->direction, character->turnSpeed);
        }
        else
        {
            character->direction = counterclockwise(character->direction, -character->turnSpeed);
        }
    }
}
void Character_turn_to_mouse(Character *character, SDL_Renderer *renderer, const Camera *camera) // 角色缓慢地面向鼠标方向(for debug)
{
    Vector headToMouse = Character_get_head_to_mouse_direction(character, renderer, camera);
    Character_turn_to_vector(character, headToMouse);
}

// 检查并处理本角色头部与另一个角色整体的碰撞,(第二个参数为自己时,为处理自己头部与身子的碰撞)
bool Character_HandleCollision(Character *self, const Character *another)
{
    if (!self || !another || self->bodyCount < 5) // 至少需要5个节点才可能自我碰撞
        return false;

    bool collisionOccurred = false;
    node *head = &self->body[0];

    // 只检查头部与身体其他部分的碰撞（除了直接相连的几个节点）
    // 跳过头部直接相连的节点以避免误检
    float biggerHeadRadius = head->radius * 1.35f * fmaxf(1.0f, fmaxf(self->headInside, fmaxf(self->head30Inside, self->head60Inside)));
    for (int i = (self == another ? 4 : 0); i < another->bodyCount; i++)
    {
        node *bodyPart = &another->body[i];

        // 计算头部与身体节点之间的距离
        float dx = head->x - bodyPart->x;
        float dy = head->y - bodyPart->y;
        float distanceSquared = dx * dx + dy * dy;

        // 计算碰撞所需的最小距离（两个半径之和）
        float collisionDistance = biggerHeadRadius + bodyPart->radius;
        float collisionDistanceSquared = collisionDistance * collisionDistance;

        // 如果距离小于碰撞距离，则发生碰撞
        if (distanceSquared < collisionDistanceSquared && distanceSquared > 0)
        {
            collisionOccurred = true;

            // 计算实际距离
            float distance = sqrtf(distanceSquared);

            // 计算分离向量（从身体节点指向头部）
            float separationX = dx / distance;
            float separationY = dy / distance;

            // 计算需要分离的距离
            float penetrationDepth = collisionDistance - distance;

            // 将头部和身体节点分开（各移动一半距离）
            // 注意：这里我们只移动头部，因为身体其他部分由Character_update维护
            head->x += separationX * penetrationDepth * 0.5f;
            head->y += separationY * penetrationDepth * 0.5f;
        }
    }

    return collisionOccurred;
}

// 根据方向和速度更新角色的身体节点,然后更新渲染盒(渲染盒永远需要更新,规定角色生成后就始终要检测是否需要渲染,没有隐身状态)
void Character_update(Character *character, CharacterPool *pool, bool isPlayer)
{
    // 添加空指针检查
    if (!character || !character->body || !pool) return;

    // 移动头节点,然后更新身体节点
    vector_Normalization(&character->direction);
    character->body[0].x += character->speed * character->direction.x;
    character->body[0].y += character->speed * character->direction.y;
    for (int i = 1; i < character->bodyCount; i++)
    {
        // 更新身体节点的位置
        Vector direction_Last_To_Now = vector_get(character->body[i - 1].x, character->body[i - 1].y, character->body[i].x, character->body[i].y);
        // 限制身体段之间的夹角(脊椎最大灵活度),核心作用:改变direction_Last_To_Now,在角度超过限制的情况下
        if (i >= 2)
        {
            Vector direction_2Last_To_Last = vector_get(character->body[i - 2].x, character->body[i - 2].y, character->body[i - 1].x, character->body[i - 1].y);
            float cosOriginalAngle = vector_dot(direction_Last_To_Now, direction_2Last_To_Last) / (vector_norm(direction_Last_To_Now) * vector_norm(direction_2Last_To_Last));
            float cosMaxAngle = cosf(Angle_To_Rad(character->body[i - 1].flexibility));
            if (cosOriginalAngle < cosMaxAngle)
            {
                Vector counterclockwiseAngle = counterclockwise(direction_2Last_To_Last, character->body[i - 1].flexibility);
                Vector clockwiseAngle = counterclockwise(direction_2Last_To_Last, -character->body[i - 1].flexibility);
                if (vector_dot(counterclockwiseAngle, direction_Last_To_Now) > vector_dot(clockwiseAngle, direction_Last_To_Now))
                {
                    direction_Last_To_Now = counterclockwiseAngle;
                }
                else
                {
                    direction_Last_To_Now = clockwiseAngle;
                }
            }
        }
        // 获取当前身体节点的新位置
        SDL_FPoint newPoint = Get_FPoint_From_parametric_equation((SDL_FPoint){character->body[i - 1].x, character->body[i - 1].y}, direction_Last_To_Now, character->body[i - 1].distance);
        character->body[i].x = newPoint.x;
        character->body[i].y = newPoint.y;
    }

    // 更新自己的碰撞盒
    Character_UpdateAABBBox(character);
    // 更新渲染盒
    character->renderBox.minX = character->box.minX - 50;
    character->renderBox.maxX = character->box.maxX + 50;
    character->renderBox.minY = character->box.minY - 50;
    character->renderBox.maxY = character->box.maxY + 50;
    // 处理与自己(从第4个身体节点开始检测)和其它角色(全部的身体节点)的碰撞
    for (int i = 0; i < CharacterPool_Size(pool); i++)
    {
        Character *other = CharacterPool_Get(pool, i);
        // 添加空指针检查
        if (!other) continue;
        if (AABBBoxCollision(character->box, other->box))
        {
            bool collision = Character_HandleCollision(character, other);
            if (!isPlayer && i == 0 && collision)
            {
                other->HP -= 0.1f;
            }
        }
    }

    // 更新枪的位置
    if (character->haveHeadGun)
    {
        SDL_FPoint finalPos = Get_FPoint_From_parametric_equation((SDL_FPoint){character->body[0].x, character->body[0].y}, character->direction, character->body[0].radius + character->headGun.bullet.radius);
        character->headGun.x = finalPos.x;
        character->headGun.y = finalPos.y;
        character->headGun.direction = character->direction;
    }
    if (character->haveTailGun)
    {
        node tail = character->body[character->bodyCount - 1];
        node beforeTail = character->body[character->bodyCount - 2];
        Vector tailDir = vector_get(beforeTail.x, beforeTail.y, tail.x, tail.y);
        vector_Normalization(&tailDir);
        SDL_FPoint finalPos = Get_FPoint_From_parametric_equation((SDL_FPoint){tail.x, tail.y}, tailDir, tail.radius + character->tailGun.bullet.radius);
        character->tailGun.direction = tailDir;
        character->tailGun.x = finalPos.x;
        character->tailGun.y = finalPos.y;
    }

    // 处理腿的运动(如果type是蜥蜴的话)
    if (character->type == LIZARD)
    {
        // 添加边界检查，确保访问的节点存在
        if (character->bodyCount > character->frontLegPos && character->bodyCount > character->backLegPos && character->frontLegPos > 0 && character->backLegPos > 0)
        {

            for (int i = 0; i < 4; i++)
            {
                Vector bodyDir;
                int pos = (i <= 1 ? character->frontLegPos : character->backLegPos);

                bodyDir = vector_get(character->body[pos].x, character->body[pos].y, character->body[pos - 1].x, character->body[pos - 1].y);
                character->legs[i].root = (SDL_FPoint){character->body[pos].x, character->body[pos].y};
                Vector xDir;

                if (i % 2 == 0) // 左腿
                {
                    xDir = counterclockwise_90(bodyDir);
                }
                else
                {
                    xDir = clockwise_90(bodyDir);
                }
                float legMaxDistance = (i <= 1 ? character->frontLegMaxDistance : character->backLegMaxDistance);
                float legoutDistance = (i <= 1 ? character->legOutDistance : character->legOutDistanceBack);
                SDL_FPoint outPoint = Get_FPoint_From_parametric_equation((SDL_FPoint){character->body[pos].x, character->body[pos].y}, xDir, legoutDistance);
                SDL_FPoint finalPoint = Get_FPoint_From_parametric_equation(outPoint, bodyDir, legMaxDistance);
                float dx = finalPoint.x - character->legs[i].head.x;
                float dy = finalPoint.y - character->legs[i].head.y;
                float disSquared = dx * dx + dy * dy;
                float maxDisSquared = legMaxDistance * legMaxDistance;
                if (disSquared < maxDisSquared) // 如果不满足踏步条件,final改成当前的head
                {
                    finalPoint = character->legs[i].head;
                }
                if (Chain3_Update(&character->legs[i], character->legs[i].root, finalPoint, (i <= 1 ? bodyDir : negate_vector(bodyDir))))
                {
                    Chain3_Update(&character->legs[i], character->legs[i].root, finalPoint, (i <= 1 ? bodyDir : negate_vector(bodyDir)));
                }
            }
        }
    }
}

// 角色运动部分
void Character_speed_add(Character *character, float addSpeed)
{
    character->speed += addSpeed;
    character->speed = SDL_clamp(character->speed, character->minSpeed, character->maxSpeed);
}
void Character_speed_set(Character *character, float setSpeed)
{
    character->speed = setSpeed;
    character->speed = SDL_clamp(character->speed, character->minSpeed, character->maxSpeed);
}
void Character_turn_left(Character *character) { character->direction = counterclockwise(character->direction, character->turnSpeed); }
void Character_turn_right(Character *character) { character->direction = counterclockwise(character->direction, -character->turnSpeed); }
//  ---------------------角色池部分----------------------------------
//  初始化角色池
void CharacterPool_Init(CharacterPool *pool, int initialCapacity)
{
    if (!pool) return;

    pool->capacity = initialCapacity;
    pool->size = 0;
    pool->characters = (Character **)malloc(initialCapacity * sizeof(Character *));

    if (!pool->characters)
    {
        fprintf(stderr, "Failed to allocate memory for character pool\n");
        exit(1);
    }

    // 初始化指针数组为NULL
    memset(pool->characters, 0, initialCapacity * sizeof(Character *));
}

// 向角色池中添加角色
void CharacterPool_Add(CharacterPool *pool, Character *character)
{
    if (!pool || !character) return;

    // 如果需要扩容
    if (pool->size >= pool->capacity)
    {
        int newCapacity = pool->capacity * 2;
        Character **temp = (Character **)realloc(pool->characters, newCapacity * sizeof(Character *));

        if (!temp)
        {
            fprintf(stderr, "Failed to resize character pool\n");
            return;
        }

        pool->characters = temp;
        pool->capacity = newCapacity;

        // 初始化新增的空间为NULL
        for (int i = pool->size; i < pool->capacity; i++)
        {
            pool->characters[i] = NULL;
        }
    }

    pool->characters[pool->size] = character;
    pool->size++;
}

// 删除角色池中一个角色(改变顺序):先销毁要删除的角色,然后让i处的指针直接指向最后一个角色,然后把最后一个位置设为NULL使其无法继续掌管之前的数据,最后减少计数器
void CharacterPool_Remove(CharacterPool *pool, int index)
{
    if (!pool || index < 0 || index >= pool->size || pool->size == 0)
    {
        return;
    }

    // 先销毁要删除的角色
    Character_Destroy(pool->characters[index]);

    // 将最后一个元素移到被删除的位置
    pool->characters[index] = pool->characters[pool->size - 1];

    // 将最后一个位置设为NULL并减少计数
    pool->characters[pool->size - 1] = NULL;
    pool->size--;
}

// 从角色池中获取角色
Character *CharacterPool_Get(const CharacterPool *pool, int index)
{
    if (!pool || index < 0 || index >= pool->size)
    {
        return NULL;
    }

    return pool->characters[index];
}

// 获取角色池大小
int CharacterPool_Size(const CharacterPool *pool)
{
    if (!pool) return 0;
    return pool->size;
}

// 更新角色池中的所有角色
void CharacterPool_Update(CharacterPool *pool)
{
    if (!pool) return;

    for (int i = 0; i < pool->size; i++)
    {
        Character *character = pool->characters[i];
        if (character)
        {
            Character_update(character, pool, (i == 0));
        }
    }
}

// 检查所有怪物角色血量并更新分数
void CharacterPool_Check_Enemy_HP(CharacterPool *pool, int *score)
{

    if (!pool) return;

    for (int i = 1; i < pool->size; i++)
    {
        Character *character = pool->characters[i];
        if (character && character->HP <= 0)
        {
            *score += character->maxHP;
            CharacterPool_Remove(pool, i);
        }
    }
}

// 渲染角色池中的所有角色
void CharacterPool_Render(CharacterPool *pool, SDL_Renderer *renderer, const Camera *camera, BatchRenderer *batch)
{
    if (!pool || !renderer || !camera || !batch) return;

    for (int i = 0; i < pool->size; i++)
    {
        Character *character = pool->characters[i];
        if (character)
        {
            Character_check_render(character, camera);
            Character_render(character, renderer, camera, batch);
        }
    }
}

// 清理角色池
void CharacterPool_Clear(CharacterPool *pool)
{
    if (!pool) return;

    for (int i = 0; i < pool->size; i++)
    {
        if (pool->characters[i])
        {
            Character_Destroy(pool->characters[i]);
            pool->characters[i] = NULL;
        }
    }

    pool->size = 0;
}

// 销毁角色池中的所有角色并释放内存
void CharacterPool_Destroy(CharacterPool *pool)
{
    if (!pool) return;

    // 销毁所有角色
    for (int i = 0; i < pool->size; i++)
    {
        if (pool->characters[i])
        {
            Character_Destroy(pool->characters[i]);
            pool->characters[i] = NULL;
        }
    }

    // 释放指针数组内存
    if (pool->characters)
    {
        free(pool->characters);
        pool->characters = NULL;
    }

    pool->size = 0;
    pool->capacity = 0;
}

//----------------------三节点链条部分----------------------------------
SDL_FPoint Constrain(SDL_FPoint from, SDL_FPoint to, float distance)
{
    Vector direction_To_From = vector_get(to.x, to.y, from.x, from.y);
    return Get_FPoint_From_parametric_equation(to, direction_To_From, distance);
}
bool Chain3_Update(Chain3 *chain, SDL_FPoint rootPosition, SDL_FPoint finalPosition, Vector aimDirection)
{
    // 添加空指针检查
    if (!chain) return false;

    // 保存更新前的中间点位置，用于判断是否需要翻转
    SDL_FPoint oldMiddle = chain->middle;

    for (int i = 0; i < 2; i++)
    {
        // 正向去
        chain->head = finalPosition;
        chain->middle = Constrain(chain->middle, chain->head, chain->distanceMH);
        chain->root = Constrain(chain->root, chain->middle, chain->distanceMR);

        // 反向回
        chain->root = rootPosition;
        chain->middle = Constrain(chain->middle, chain->root, chain->distanceMR);
        chain->head = Constrain(chain->head, chain->middle, chain->distanceMH);
    }

    // 设置脚朝向
    Vector middleToRoot = vector_get(chain->middle.x, chain->middle.y, chain->root.x, chain->root.y);
    Vector middleToHead = vector_get(chain->middle.x, chain->middle.y, chain->head.x, chain->head.y);
    Vector middleDraw = (Vector){middleToRoot.x + middleToHead.x, middleToRoot.y + middleToHead.y};
    vector_Normalization(&middleDraw);
    vector_Normalization(&aimDirection);

    if (vector_dot(middleDraw, aimDirection) < 0)
    {

        SDL_FPoint p1 = chain->root;
        SDL_FPoint p2 = chain->head;
        float A = p2.y - p1.y;
        float B = p1.x - p2.x;
        float C = p2.x * p1.y - p1.x * p2.y;
        float up = A * chain->middle.x + B * chain->middle.y + C;
        float down = A * A + B * B;

        // 检查分母是否为0，避免除零错误
        if (down > 0.0001f)
        {
            SDL_FPoint newMiddle;
            newMiddle.x = chain->middle.x - 2 * A * up / down;
            newMiddle.y = chain->middle.y - 2 * B * up / down;
            chain->middle = newMiddle;
        }
        return true;
    }

    return false;
}

// 子弹部分
void BulletPool_Init(BulletPool *pool) { pool->bulletCount = 0; }
void BulletPool_Add(BulletPool *pool, Bullet bullet)
{
    if (pool->bulletCount == MAX_BULLET_COUNT)
    {
        pool->bullets[0] = bullet;
        return;
    }
    pool->bullets[pool->bulletCount] = bullet;
    pool->bulletCount++;
}
void BulletPool_Remove(BulletPool *pool, int index)
{
    if (index >= pool->bulletCount)
    {
        pool->bulletCount--;
        return;
    }
    pool->bullets[index] = pool->bullets[pool->bulletCount - 1];
    pool->bulletCount--;
}

// 清理子弹池
void BulletPool_Clear(BulletPool *pool)
{
    Bullet nullBullet = {};
    for (int i = 0; i < pool->bulletCount; i++)
    {
        pool->bullets[i] = nullBullet;
    }
    pool->bulletCount = 0;
}

void BulletPool_Destroy(BulletPool *pool) { BulletPool_Clear(pool); }
bool Bullet_Character_Collision(Bullet *bullet, Character *character)
{
    AABBBox bulletBox = {bullet->x - bullet->radius, bullet->x + bullet->radius, bullet->y - bullet->radius, bullet->y + bullet->radius};

    if (!AABBBoxCollision(bulletBox, character->box)) return false;
    for (int i = 0; i < character->bodyCount; i++)
    {
        float dx = character->body[i].x - bullet->x;
        float dy = character->body[i].y - bullet->y;
        float CollisionDis = character->body[i].radius + bullet->radius;
        if (dx * dx + dy * dy <= CollisionDis * CollisionDis)
        {
            return true;
        }
    }
    return false;
}
void damage(Character *character, Bullet *bullet, float k) { character->HP -= bullet->damage * k; }
bool Bullet_Update(Bullet *bullet, CharacterPool *characterPool) // 返回值表示是否要删掉该子弹
{
    bool remove = false;
    bullet->x += bullet->speed * bullet->direction.x;
    bullet->y += bullet->speed * bullet->direction.y;
    AABBBox box = {bullet->x - bullet->radius, bullet->x + bullet->radius, bullet->y - bullet->radius, bullet->y + bullet->radius};
    // 先处理子弹打向自己
    Character *self = characterPool->characters[0];
    if (bullet->flyCount > 10 && AABBBoxCollision(box, self->box))
    {
        if (Bullet_Character_Collision(bullet, self))
        {
            damage(self, bullet, 0.25f);
            return true;
        }
    }
    for (int i = 1; i < characterPool->size; i++)
    {
        Character *character = characterPool->characters[i];
        if (AABBBoxCollision(box, character->box))
        {
            if (Bullet_Character_Collision(bullet, character))
            {
                damage(character, bullet, 1.0f);
                return true;
            }
        }
    }
    bullet->flyCount++;
    return bullet->flyCount > Max_FLY_COUNT;
}
void BulletPool_Update(BulletPool *pool, CharacterPool *characterPool)
{
    for (int i = 0; i < pool->bulletCount; i++)
    {
        if (Bullet_Update(&pool->bullets[i], characterPool))
        {
            BulletPool_Remove(pool, i);
        }
    }
}
void BulletPool_Render(BulletPool *pool, SDL_Renderer *renderer, const Camera *camera, BatchRenderer *batch)
{
    for (int i = 0; i < pool->bulletCount; i++)
    {
        Polygon_DrawCircle(batch, pool->bullets[i].x, pool->bullets[i].y, pool->bullets[i].radius, 10, pool->bullets[i].bulletColor, camera);
    }
}

// 枪械部分

void Gun_Try_Shoot(Gun *gun, BulletPool *pool, bool needShoot) // 每个逻辑帧都应该调用
{
    gun->delayCount++;
    if (needShoot && gun->delayCount > gun->shootDelay)
    {
        Bullet shootBullet = gun->bullet;
        // 改变属性
        shootBullet.damage *= gun->damageK;
        // 设定初始运动状态
        shootBullet.flyCount = 0;
        shootBullet.x = gun->x;
        shootBullet.y = gun->y;
        shootBullet.direction = gun->direction;

        BulletPool_Add(pool, shootBullet);
        gun->delayCount = 0;
    }
}