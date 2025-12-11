#include "bot2.h"


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>

#include <log/log.h>
#include <time.h>

#include "../../math/math.h"
#include "../../math/rand.h"
#include "../obstacle.h"
#include "../player_movement.h"
#include "../specialConditions/wind.h"
#include <stdio.h>

void bot2Main(App* app, Player* firstPlayer, Player* secondPlayer,
              int32_t* heightMap, RenderObject* projectile,
              RenderObject* explosion, SDL_bool* regenMap,
              SDL_bool* recalcBulletPath, double initGunAngle) {
  printf("call from bot2 func\n");
  Player* enemy;
  if (app->currPlayer == firstPlayer) {
    enemy = secondPlayer;
  } else {
    enemy = firstPlayer;
  }

  // getting collison for an enemy player
  SDL_Point collisionP1, collisionP2, collisionP3;
  int32_t collisionP1R, collisionP2R, collisionP3R;
  // getting player collisions centers
  // left player collision
  if (app->currPlayer == secondPlayer) {
    collisionP1 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){35 * app->scalingFactorX, 19 * app->scalingFactorY});
    collisionP2 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){20 * app->scalingFactorX, 12 * app->scalingFactorY});
    collisionP3 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){7 * app->scalingFactorX, 19 * app->scalingFactorY});
    // radiuses
    collisionP1R = 9 * MAX(app->scalingFactorX, app->scalingFactorY);
    collisionP2R = 13 * MAX(app->scalingFactorX, app->scalingFactorY);
    collisionP3R = 10 * MAX(app->scalingFactorX, app->scalingFactorY);
  }
  // right player collison
  else {
    collisionP1 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){11 * app->scalingFactorX, 19 * app->scalingFactorY});
    collisionP2 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){23 * app->scalingFactorX, 13 * app->scalingFactorY});
    collisionP3 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){36 * app->scalingFactorX, 18 * app->scalingFactorY});
    // radiuses
    collisionP1R = 11 * MAX(app->scalingFactorX, app->scalingFactorY);
    collisionP2R = 11 * MAX(app->scalingFactorX, app->scalingFactorY);
    collisionP3R = 9 * MAX(app->scalingFactorX, app->scalingFactorY);
  }

  SDL_Point initPos = getPixelScreenPosition(
      (SDL_Point){app->currPlayer->tankObj->data.texture.scaleRect.x,
                  app->currPlayer->tankObj->data.texture.scaleRect.y},
      (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
      app->currPlayer->tankObj->data.texture.angle,
      (SDL_Point){24 * app->scalingFactorX, 7 * app->scalingFactorY});

  initPos.x /= app->scalingFactorX;
  initPos.y /= app->scalingFactorY;

  initPos.x += 25 * cos(DEGTORAD(initGunAngle));
  initPos.y -= 25 * sin(DEGTORAD(initGunAngle));

  SDL_FPoint currPos = {
      .x = (float)initPos.x,
      .y = (float)initPos.y,
  };

  double velMultiplicator;
  int32_t explosionRadius;
  SDL_bool isHittableNearby;
  switch (app->currWeapon) {
    // small bullet
    case 0:
      velMultiplicator = 2;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      break;
    // BIG BULLET
    case 1:
      velMultiplicator = 1.75;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      break;
    // small boom
    case 2:
      velMultiplicator = 1.25;
      explosionRadius = projectile->data.texture.constRect.w * 2;
      isHittableNearby = SDL_TRUE;
      break;
    // BIG BOOM
    case 3:
      velMultiplicator = 1.0;
      explosionRadius = projectile->data.texture.constRect.w * 4;
      isHittableNearby = SDL_TRUE;
      break;
    default:
      velMultiplicator = 1.0;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      break;
  }
  int32_t windStrengthMin, windStrengthMax;
  getWindRange(app, &windStrengthMin, &windStrengthMax);

  int32_t initVel = app->currPlayer->firingPower * velMultiplicator;
  int32_t hitPos =
      calcHitPosition(&currPos, initVel, initGunAngle, heightMap, app,
                      &collisionP1, &collisionP2, &collisionP3, collisionP1R,
                      collisionP2R, collisionP3R, projectile, AVG(windStrengthMin, windStrengthMax));
  if (hitPos < -1) {
    SDL_Delay(200);

    shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
          regenMap);
    recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
    recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
    return;
  }
  int32_t leftCorner =
      enemy->tankObj->data.texture.constRect.x * app->scalingFactorX -
      explosionRadius;
  int32_t rightCorner =
      enemy->tankObj->data.texture.constRect.x * app->scalingFactorX +
      enemy->tankObj->data.texture.constRect.w * app->scalingFactorX *
          cos(DEGTORAD(enemy->tankAngle)) +
      explosionRadius;
  if (hitPos >= leftCorner && hitPos <= rightCorner && isHittableNearby) {
    shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
          regenMap);
    recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
    recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
    return;
  }

  SDL_bool isFinded = SDL_FALSE;
  int32_t bestAngle = 0;
  int32_t bestPower = 0;
  for (int32_t angle = 0; angle <= 120; ++angle) {
    double currAngle = app->currPlayer->tankGunObj->data.texture.angle;

    if (app->currPlayer == secondPlayer) {
      currAngle += 180 + angle;
    } else {
      currAngle += -angle;
    }
    currAngle = round(currAngle);
    currAngle = 360 - normalizeAngle(currAngle);

    for (int32_t firingPower = 0; firingPower <= 99; ++firingPower) {
      int32_t hitPos = calcHitPosition(&currPos, firingPower * velMultiplicator,
                                       currAngle, heightMap, app, &collisionP1,
                                       &collisionP2, &collisionP3, collisionP1R,
                                       collisionP2R, collisionP3R, projectile, AVG(windStrengthMin, windStrengthMax));
      if (hitPos < -1 && angle>bestAngle) {
        bestAngle = angle;
        bestPower = firingPower;
        isFinded = SDL_TRUE;
      }
    }
  }
  if (isFinded) {
    smoothChangeAngle(app->currPlayer, bestAngle, &app->currState,
                      recalcBulletPath);
    smoothChangePower(app->currPlayer, bestPower, &app->currState,
                      recalcBulletPath);
    SDL_Delay(200);
    shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
          regenMap);
    recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
    recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
  } 
  else if (app->currPlayer->movesLeft != 0)
  {
    if (app->currPlayer == firstPlayer) {
      smoothMove(app, SDL_TRUE, SDL_TRUE, heightMap, obstacles);
    } else {
      smoothMove(app, SDL_FALSE, SDL_FALSE, heightMap, obstacles);
    }
  }

    shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
          regenMap);
    recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
    recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
  return;
}
