#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "../App.h"
#include "obstacle_struct.h"

#define MAXSTONES 2
#define MAXCLOUDS 10

RenderObject* createCloud(App* app, int32_t* heightmap, int32_t startPos,
                          int32_t endPos, int32_t probability,
                          uint32_t currCloudCnt);

RenderObject* createStone(App* app, int32_t* heightmap, int32_t startPos,
                          int32_t endPos, uint32_t currStoneCnt);

RenderObject* createTree(App* app, int32_t* heightmap, int32_t startPos,
                         int32_t endPos, int32_t probability);

SDL_bool checkObstacleCollisions(uint32_t currX, uint32_t currY);

extern obstacleStruct obstacles[MAXSTONES + MAXCLOUDS];
#endif