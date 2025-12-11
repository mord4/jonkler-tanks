#include "obstacle.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL_rect.h>
#include <stdbool.h>

#include <log/log.h>

#include "../SDL/SDL_render.h"
#include "../math/math.h"
#include "../math/rand.h"
#include "obstacle_struct.h"

obstacleStruct obstacles[MAXSTONES + MAXCLOUDS];
SDL_Point center = {0, 0};
    // the freak was fall func name??
static RenderObject* renderStoneWithAngle(App* app, int32_t* heightmap,
                                          int32_t x) {
  SDL_Point pos = {x, -66 + app->screenHeight / app->scalingFactorY -
                          heightmap[(int32_t)(x * app->scalingFactorX)] /
                              app->scalingFactorY};
  double angle =
      getAngle(x * app->scalingFactorX, heightmap, 95 * app->scalingFactorX);

  RenderObject* object = createRenderObject(
      app->renderer, TEXTURE | EXTENDED, 0, b_NONE, "media/imgs/rock.png", &pos,
      360 - angle, SDL_FLIP_NONE, &center);
  return object;
}

// create a single cloud with 1 hp and prob. (1-probability)
RenderObject* createCloud(App* app, int32_t* heightmap, int32_t startPos,
                          int32_t endPos, int32_t probability,
                          uint32_t currCloudCnt) {
  int imMrKrabsAndILikeMoney = getRandomValue(0, 100);
  RenderObject* res = NULL;

  if (imMrKrabsAndILikeMoney > probability) {
    int x = getRandomValue(startPos, endPos);

    res = createRenderObject(
        app->renderer, TEXTURE, 1, b_NONE, "media/imgs/cloud.png",
        &(SDL_Point){x,
                     -getRandomValue(200, 400) +
                         app->screenHeight / app->scalingFactorY -
                         heightmap[(int32_t)((x + 54) * app->scalingFactorX)] /
                             app->scalingFactorY});
  }
  obstacles[MAXSTONES + currCloudCnt].obstacleObject = res;
  obstacles[MAXSTONES + currCloudCnt].health = 1;

  return res;
}

// create a single stone with 3 hp
RenderObject* createStone(App* app, int32_t* heightmap, int32_t startPos,
                          int32_t endPos, uint32_t currStoneCnt) {
  int x = getRandomValue(startPos, endPos);
  RenderObject* res = renderStoneWithAngle(app, heightmap, x);
  obstacles[currStoneCnt].obstacleObject = res;
  obstacles[currStoneCnt].health = 3;

  return res;
}

// create a single tree with prob. (1-probability)
RenderObject* createTree(App* app, int32_t* heightmap, int32_t startPos,
                         int32_t endPos, int32_t probability) {
  int imMrKrabsAndILikeMoney = getRandomValue(0, 100);
  RenderObject* res = NULL;

  if (imMrKrabsAndILikeMoney > probability) {
    int x = getRandomValue(startPos, endPos);

    res = createRenderObject(
        app->renderer, TEXTURE, 1, b_NONE, "media/imgs/tree1.png",
        &(SDL_Point){x,
                     -120 + app->screenHeight / app->scalingFactorY -
                         heightmap[(int32_t)((x + 54) * app->scalingFactorX)] /
                             app->scalingFactorY});
  }

  return res;
}

// checking for all obstacle collisions
// and removing health
// returns true if some obstacle was destroyed
SDL_bool checkObstacleCollisions(uint32_t currX, uint32_t currY,
                                 SDL_bool isEmulating) {
  for (uint32_t i = 0; i != MAXSTONES + MAXCLOUDS; ++i) {
    // skipping non existing objects or alredy destroyed objects
    if (obstacles[i].obstacleObject == NULL || obstacles[i].health == 0) {
      continue;
    }
    int obstacleX = obstacles[i].obstacleObject->data.texture.constRect.x;
    int obstacleW = obstacles[i].obstacleObject->data.texture.constRect.w;

    int obstacleY = obstacles[i].obstacleObject->data.texture.constRect.y;
    int obstacleH = obstacles[i].obstacleObject->data.texture.constRect.h;

    SDL_Rect obstacleRect = {
        .x = obstacleX, .y = obstacleY, .h = obstacleH, .w = obstacleW};

    if (i >= MAXSTONES) {
      if (SDL_PointInRect(&(SDL_Point){currX, currY}, &obstacleRect)) {
        if (!isEmulating && --obstacles[i].health == 0) {
          // hiding destroyed objects
          obstacles[i].obstacleObject->disableRendering = SDL_TRUE;
        }
        return SDL_TRUE;
      }
    } else if (PointInRotatedRect(
                   &(obstacleRect), &(SDL_Point){currX, currY},
                   360 - obstacles[i].obstacleObject->data.texture.angle)) {
      if (!isEmulating && --obstacles[i].health == 0) {
        // hiding destroyed objects
        obstacles[i].obstacleObject->disableRendering = SDL_TRUE;
      }
      return SDL_TRUE;
    }
  }
  return SDL_FALSE;
}