#ifndef CHARACTER_H
#define CHARACTER_H

#define DEFALUT_MAX_TURN_SPEED 5.0f
#define DEFALUT_MAX_SPEED 10.0f
#define DEFALUT_MIN_SPEED -5.0f
#define DEFALUT_SPEED 0.0f
#define DEFALUT_DIRECTION_X 1.0f
#define DEFALUT_DIRECTION_Y 0.0f
#define DEFALUT_OUTLINE_WIDTH 3.0f

#define MAX_BULLET_COUNT 5000
#define Max_FLY_COUNT 6000

#define CUTTING_DISTANCE 0.25f

#include "batchingRender.h"
#include "camera.h"
#include "vector.h"
#include <SDL3/SDL.h>
typedef struct
{
    float minX, maxX, minY, maxY;
} AABBBox;
enum CharacterType
{
    SNAKE, // 0
    LIZARD // 1
};
typedef struct // 怪物只追我近战,不发射子弹,子弹只打怪(打到自己掉四分之一的血)
{
    // 属性
    float damage;
    float speed; // 子弹飞行速度
    float radius;
    SDL_FColor bulletColor;

    // 运动状态
    int flyCount; // 已经飞行了多少帧,超过一定值就删掉
    Vector direction;
    float x, y;
} Bullet; // 决定了伤害,速度,大小,颜色
typedef struct
{
    int bulletCount;
    Bullet bullets[MAX_BULLET_COUNT]; // 最多有MAX_BULLET_COUNT个子弹同时出现,不再动态更新长度,简化
} BulletPool;
enum GunType
{
    SHORTGUN,
    LONGGUN,
    SNIPERGUN
};
enum BulletType
{
    AMMO,
    BUBBLE,
    SNIPERBULLET
};
typedef struct
{
    enum GunType type; // 枪械类型,负责渲染对应纹理
    Bullet bullet;
    float shootDelay; // 间隔几个逻辑帧发射一个子弹
    float damageK;    // 伤害系数,子弹的伤害会乘以这个系数
    int delayCount;   // 距离上一次射击已经过去了多少帧
    float x, y;       // 枪口位置
    Vector direction; // 枪口方向

} Gun; // 决定了射速和伤害系数(在发射子弹之后就更改子弹的伤害,乘上系数)
typedef struct
{
    float x;
    float y;
    float radius;
    float distance;    // 约束下一个点的距离,它后面一个点必须在以这个距离为半径的圆上
    float flexibility; // 单位是角度,当前节点作为两段身体连接处时的最大灵活度(头尾节点没有),两个身体段形成的锐夹角的最大角度对应的角度(锐夹角越大身体越弯曲)
} node;
typedef struct
{
    SDL_FPoint root;
    SDL_FPoint middle;
    SDL_FPoint head;
    float rootRadius;
    float middleRadius;
    float headRadius;
    float distanceMR; // middle到root的距离
    float distanceMH; // middle到head的距离
} Chain3;
typedef struct // 头部的方向应该采取坦克炮塔的模式,有最大转弯速度,向着鼠标所在方向慢慢转向,能解决头部转弯太快导致的鬼畜
{
    // 身体部分
    enum CharacterType type;
    node *body; // 用于检测碰撞,更新renderBody,更新渲染盒的身体
    int bodyCount;
    Chain3 legs[4];            // 四条腿(0,1是前腿,2,3是后腿)
    int frontLegPos;           // 前腿在第几个节点处
    int backLegPos;            // 后腿在第几个节点处
    float frontLegMaxDistance; // 前腿最大落后距离,超过了就要移动腿到新的位置
    float backLegMaxDistance;  // 后腿最大落后距离,超过了就要移动腿到新的位置
    float legOutDistance;      // 腿外展的长度
    float forwardDis[2];       // 腿目标位置相对与身体段向前的距离,0为前腿,1为后腿
    float legOutDistanceBack;  // 后腿外展的长度
    float headInside;          // 头部第一个点向外伸展的长度,默认为100%
    float head30Inside;        // 头部第二个点向外伸展的长度,默认为100%
    float head60Inside;        // 头部第三个点向外伸展的长度,默认为100%
    Vector direction;          // 方向向量
    float turnSpeed;           // 转向速度(角度/帧)
    float speed;               // 速度的大小
    float maxSpeed;            // 最大速度(前进的)
    float minSpeed;            // 最小速度(后退的)

    // 渲染部分
    AABBBox box;       // 碰撞盒
    AABBBox renderBox; // 渲染盒
    bool needRender;
    float eyeInside;         // 眼睛从头节点半径向外伸展的长度,默认为80%
    float eyeRadius;         // 眼睛的半径大小
    SDL_FColor eyeColor;     // 眼睛的颜色
    SDL_FColor color;        // 身体的颜色
    SDL_FColor outLineColor; // 轮廓的颜色

    // 状态部分
    float HP;
    float maxHP;
    bool haveHeadGun; // 是否有头枪
    bool haveTailGun; // 是否有尾枪
    Gun headGun;
    Gun tailGun;
} Character;
typedef struct
{
    int capacity;           // 容量: 最多能容纳的角色数量
    int size;               // 当前角色数量
    Character **characters; // 角色指针数组
} CharacterPool;
void Render_Texture(SDL_Renderer *renderer, SDL_Texture *texture, SDL_FPoint pos, float angle, float scale);
bool AABBBoxCollision(AABBBox a, AABBBox b);
AABBBox Rect_To_AABBBox(SDL_FRect rect);
SDL_FRect AABBBox_To_Rect(AABBBox box);
Character *Character_Creat(enum CharacterType type, float x, float y, Vector initialDirection, float initialSpeed, const float *radiusList, const float *distanceList, const float *flexibility, const int bodyCount, SDL_FColor color, SDL_FColor outLineColor, const Chain3 legs[2]); // 头部初始位置,初始方向向量,初始速度向量,半径列表,约束距离列表,身体节点数量,(应确保半径列表和约束距离列表的长度一致且等于身体节点数量)(legs里面0为前腿,1为后退)
void Character_Destroy(Character *character);                                                                                                                                                                                                                                           // 销毁角色(死亡即销毁,生成新的角色重新初始化一个就行)

void Character_check_render(Character *character, const Camera *camera); // 检测是否需要渲染--渲染盒是否和摄像机视口相交

void AddPointToOutline(SDL_FPoint *points, int *count, SDL_FPoint p);

void Character_render(const Character *character, SDL_Renderer *renderer, const Camera *camera, BatchRenderer *batch); // 从头到尾遍历角色的身体节点,运用平滑算法生成更多的顶点,按顺序画三角形即可;头尾的节点特殊处理(仅使用圆上的更多点)

// 渲染枪
void Gun_render(const Gun *gun, Vector direction, SDL_Renderer *renderer, const Camera *camera, SDL_Texture *textureList[3]);
// 获取角色头部到鼠标的向量
Vector Character_get_head_to_mouse_direction(const Character *character, SDL_Renderer *renderer, const Camera *camera);
void Character_turn_to_vector(Character *character, Vector direction);
void Character_directly_turn_to_mouse(Character *character, SDL_Renderer *renderer, const Camera *camera); // 角色直接面向鼠标方向(for debug)
void Character_turn_to_mouse(Character *character, SDL_Renderer *renderer, const Camera *camera);          // 角色缓慢地面向鼠标方向
void Character_update(Character *character, CharacterPool *pool, bool isPlayer);                           // 根据方向和速度更新角色的身体节点,然后更新渲染盒(渲染盒永远需要更新,规定角色生成后就始终要检测是否需要渲染,没有隐身状态)
void Character_speed_add(Character *character, float addSpeed);                                            // 增加速度,注意速度限制
void Character_speed_set(Character *character, float setSpeed);                                            // 设置速度,注意速度限制
void Character_turn_left(Character *character);                                                            //
void Character_turn_right(Character *character);                                                           //
void Character_direction_set(Character *character, Vector direction);                                      // 设置方向,注意最后要归一化,确保方向永远是单位向量,模为1

bool Character_HandleCollision(Character *self, const Character *another); // 检查并处理本角色头部与另一个角色整体的碰撞,(第二个参数为自己时,为处理自己头部与身子的碰撞)

// 角色池部分
// 初始化角色池
void CharacterPool_Init(CharacterPool *pool, int initialCapacity);

// 向角色池中添加角色
void CharacterPool_Add(CharacterPool *pool, Character *character);

// 删除角色池中一个角色(改变顺序)
void CharacterPool_Remove(CharacterPool *pool, int index);

// 从角色池中获取角色
Character *CharacterPool_Get(const CharacterPool *pool, int index);

// 获取角色池大小
int CharacterPool_Size(const CharacterPool *pool);

// 更新角色池中的所有角色
void CharacterPool_Update(CharacterPool *pool);

// 检查所有怪物角色血量并更新分数
void CharacterPool_Check_Enemy_HP(CharacterPool *pool, int *score);

// 渲染角色池中的所有角色
void CharacterPool_Render(CharacterPool *pool, SDL_Renderer *renderer, const Camera *camera, BatchRenderer *batch);
// 清理角色池
void CharacterPool_Clear(CharacterPool *pool);
// 销毁角色池中的所有角色并释放内存
void CharacterPool_Destroy(CharacterPool *pool);

//----------------------三节点链条部分----------------------------------
SDL_FPoint Constrain(SDL_FPoint from, SDL_FPoint to, float distance);
bool Chain3_Update(Chain3 *chain, SDL_FPoint rootPosition, SDL_FPoint finalPosition, Vector aimDirection);

//----------子弹部分---------
void BulletPool_Init(BulletPool *pool);
void BulletPool_Add(BulletPool *pool, Bullet bullet);
void BulletPool_Remove(BulletPool *pool, int index);
void BulletPool_Clear(BulletPool *pool);
void BulletPool_Destroy(BulletPool *pool);
bool Bullet_Character_Collision(Bullet *bullet, Character *character);
void damage(Character *character, Bullet *bullet, float k);
bool Bullet_Update(Bullet *bullet, CharacterPool *characterPool);
void BulletPool_Update(BulletPool *pool, CharacterPool *characterPool);
void BulletPool_Render(BulletPool *pool, SDL_Renderer *renderer, const Camera *camera, BatchRenderer *batch);

// ------------枪械部分-------------
void Gun_Try_Shoot(Gun *gun, BulletPool *pool, bool needShoot);
#endif // CHARACTER_H
