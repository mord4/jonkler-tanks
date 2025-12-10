#include "bot.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>

#include <float.h>
#include <math.h>

#include "../math/math.h"
#include "player_movement.h"

#include "customBots/bot1.h"
#include "customBots/bot2.h"
#include "customBots/bot3.h"

static Player* whoIsEnenmy(Player* currPlayer, Player* firstPlayer,
                           Player* secondPlayer) {
  if (currPlayer == firstPlayer) return secondPlayer;
  return firstPlayer;
}

static void setPlayerCollision(App* app, Player* enemy, Player* firstPlayer,
                               Player* secondPlayer, SDL_Point* collisionP1,
                               SDL_Point* collisionP2, SDL_Point* collisionP3,
                               int32_t* collisionP1R, int32_t* collisionP2R,
                               int32_t* collisionP3R) {
  if (app->currPlayer == secondPlayer) {
    *collisionP1 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){35 * app->scalingFactorX, 19 * app->scalingFactorY});
    *collisionP2 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){20 * app->scalingFactorX, 12 * app->scalingFactorY});
    *collisionP3 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){7 * app->scalingFactorX, 19 * app->scalingFactorY});
    // radiuses
    *collisionP1R = 9 * MAX(app->scalingFactorX, app->scalingFactorY);
    *collisionP2R = 13 * MAX(app->scalingFactorX, app->scalingFactorY);
    *collisionP3R = 10 * MAX(app->scalingFactorX, app->scalingFactorY);
  }
  // right player collison
  else {
    *collisionP1 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){11 * app->scalingFactorX, 19 * app->scalingFactorY});
    *collisionP2 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){23 * app->scalingFactorX, 13 * app->scalingFactorY});
    *collisionP3 = getPixelScreenPosition(
        (SDL_Point){enemy->tankObj->data.texture.scaleRect.x,
                    enemy->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemy->tankObj->data.texture.angle,
        (SDL_Point){36 * app->scalingFactorX, 18 * app->scalingFactorY});
    // radiuses
    *collisionP1R = 11 * MAX(app->scalingFactorX, app->scalingFactorY);
    *collisionP2R = 11 * MAX(app->scalingFactorX, app->scalingFactorY);
    *collisionP3R = 9 * MAX(app->scalingFactorX, app->scalingFactorY);
  }
}

static void setWeaponStats(int32_t currWeapon, RenderObject* projectile,
                           double* velMultiplicator, int32_t* explosionRadius,
                           SDL_bool* isHittableNearby, int32_t* maxPower) {
  switch (currWeapon) {
    // small bullet
    case 0:
      *velMultiplicator = 2;
      *explosionRadius = projectile->data.texture.constRect.w;
      *isHittableNearby = SDL_FALSE;
      *maxPower = 99;
      break;
    // BIG BULLET
    case 1:
      *velMultiplicator = 1.75;
      *explosionRadius = projectile->data.texture.constRect.w;
      *isHittableNearby = SDL_FALSE;
      *maxPower = 99;
      break;
    // small boom
    case 2:
      *velMultiplicator = 1.25;
      *explosionRadius = projectile->data.texture.constRect.w * 2;
      *isHittableNearby = SDL_TRUE;
      *maxPower = 99;
      break;
    // BIG BOOM
    case 3:
      *velMultiplicator = 1.0;
      *explosionRadius = projectile->data.texture.constRect.w * 4;
      *isHittableNearby = SDL_TRUE;
      *maxPower = 99;
      break;
    default:
      *velMultiplicator = 1.0;
      *explosionRadius = projectile->data.texture.constRect.w;
      *isHittableNearby = SDL_FALSE;
      break;
  }
}

static int32_t justSimpleBot(App* app, Player* firstPlayer,
                             Player* secondPlayer, int32_t* heightMap,
                             RenderObject* projectile, RenderObject* explosion,
                             SDL_bool* regenMap, SDL_bool* recalcBulletPath,
                             double initGunAngle) {
  Player* enemy = whoIsEnenmy(app->currPlayer, firstPlayer, secondPlayer);

  SDL_Point collisionP1, collisionP2, collisionP3;
  int32_t collisionP1R, collisionP2R, collisionP3R;

  setPlayerCollision(app, enemy, firstPlayer, secondPlayer, &collisionP1,
                     &collisionP2, &collisionP3, &collisionP1R, &collisionP2R,
                     &collisionP3R);

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
  int32_t maxPower;

  setWeaponStats(app->currWeapon, projectile, &velMultiplicator,
                 &explosionRadius, &isHittableNearby, &maxPower);

  for (int32_t angle = 0; angle <= 120; ++angle) {
    double currAngle = app->currPlayer->tankGunObj->data.texture.angle;

    if (app->currPlayer == secondPlayer)
      currAngle += 180 + angle;
    else
      currAngle += -angle;

    currAngle = round(currAngle);
    currAngle = 360 - normalizeAngle(currAngle);

    for (int32_t power = 0; power <= maxPower; ++power) {
      int32_t hitPos = calcHitPosition(&currPos, power * velMultiplicator,
                                       currAngle, heightMap, app, &collisionP1,
                                       &collisionP2, &collisionP3, collisionP1R,
                                       collisionP2R, collisionP3R, projectile);
      // collision hit
      if (hitPos < -1) {
        smoothChangeAngle(app->currPlayer, angle, &app->currState,
                          recalcBulletPath);
        smoothChangePower(app->currPlayer, power, &app->currState,
                          recalcBulletPath);
        SDL_Delay(200);
        shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
              regenMap);
        recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
        recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
        return 0;
      }
    }
  }
  smoothChangeAngle(app->currPlayer, app->currPlayer->gunAngle, &app->currState,
                    recalcBulletPath);
  smoothChangePower(app->currPlayer, app->currPlayer->firingPower,
                    &app->currState, recalcBulletPath);
  SDL_Delay(200);
  shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
        regenMap);
  recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
  recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
  return 0;
}

void botMain(App* app, Player* player1, Player* player2, int32_t* heightMap,
             RenderObject* projectile, RenderObject* explosion,
             SDL_bool* regenMap, SDL_bool* recalcBulletPath,
             enum PlayerType playerType) {
  double initGunAngle = app->currPlayer->tankGunObj->data.texture.angle;

  // calculating angle specifically for a current player
  if (app->currPlayer == player2) {
    initGunAngle += 180 - app->currPlayer->tankGunObj->data.texture.angleAlt;
  } else {
    initGunAngle += app->currPlayer->tankGunObj->data.texture.angleAlt;
  }

  // normalizing just to be sure its in [0;2pi) and now its counterclockwise
  initGunAngle = 360 - normalizeAngle(initGunAngle);

  if (app->currState == PLAY) {
    switch (playerType) {
#ifdef BOT1_ADDED
      case BOT1:
        bot1Main(app, player1, player2, heightMap, projectile, explosion,
                 regenMap, recalcBulletPath, initGunAngle);
        break;
#endif
#ifdef BOT2_ADDED
      case BOT2:
        justSimpleBot(app, player1, player2, heightMap, projectile, explosion,
                      regenMap, recalcBulletPath, 90);
        break;
#endif
#ifdef BOT3_ADDED
      case BOT3:
        theGothGambit(app, player1, player2, heightMap, projectile, explosion,
                      regenMap, recalcBulletPath, 90);
        break;
#endif
      default:
        justSimpleBot(app, player1, player2, heightMap, projectile, explosion,
                      regenMap, recalcBulletPath, 90);
        break;
    }
  }
}
