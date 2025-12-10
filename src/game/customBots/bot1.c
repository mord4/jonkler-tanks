#include "bot1.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <limits.h>
#include <math.h>

#include <log/log.h>
#include <time.h>

#include "../../math/math.h"
#include "../obstacle.h"
#include "../player_movement.h"
#include "../specialConditions/wind.h"

// returning X coordinate of the nearest stone
// or -1 if stone wasnt found
static SDL_Point findNearestStone(SDL_bool commingFromLeft) {
  SDL_Point res = {-1, -1};
  if (!commingFromLeft) {
    for (int32_t i = MAXSTONES - 1; i >= 0; --i) {
      // skipping non existing objects or alredy destroyed objects
      if (obstacles[i].obstacleObject == NULL || obstacles[i].health == 0) {
        continue;
      }

      int obstacleX = obstacles[i].obstacleObject->data.texture.constRect.x;
      int obstacleW = obstacles[i].obstacleObject->data.texture.constRect.w;

      res.x = obstacleX;
      res.x += obstacleW;

      int obstacleY = obstacles[i].obstacleObject->data.texture.constRect.y;

      res.y = obstacleY;

      break;
    }
  } else {
    for (int32_t i = 0; i < MAXSTONES; ++i) {
      // skipping non existing objects or alredy destroyed objects
      if (obstacles[i].obstacleObject == NULL || obstacles[i].health == 0) {
        continue;
      }

      int obstacleX = obstacles[i].obstacleObject->data.texture.constRect.x;
      res.x = obstacleX;

      int obstacleY = obstacles[i].obstacleObject->data.texture.constRect.y;

      res.y = obstacleY;
      break;
    }
  }
  return res;
}

// returning X coordinate of the nearest cloud
// or -1 if no cloud was found
static SDL_Point findNearestCloud(SDL_bool isFirstPlayer) {
  SDL_Point res = {-1, -1};
  if (!isFirstPlayer) {
    for (int32_t i = MAXCLOUDS + MAXSTONES - 1; i >= MAXSTONES; --i) {
      // skipping non existing objects or alredy destroyed objects
      if (obstacles[i].obstacleObject == NULL || obstacles[i].health == 0) {
        continue;
      }

      int obstacleX = obstacles[i].obstacleObject->data.texture.constRect.x;
      int obstacleW = obstacles[i].obstacleObject->data.texture.constRect.w;

      res.x = obstacleX;
      res.x += obstacleW;

      int obstacleY = obstacles[i].obstacleObject->data.texture.constRect.y;

      res.y = obstacleY;
      break;
    }
  } else {
    for (int32_t i = MAXSTONES; i < MAXSTONES + MAXCLOUDS; ++i) {
      // skipping non existing objects or alredy destroyed objects
      if (obstacles[i].obstacleObject == NULL || obstacles[i].health == 0) {
        continue;
      }

      int obstacleX = obstacles[i].obstacleObject->data.texture.constRect.x;
      res.x = obstacleX;

      int obstacleY = obstacles[i].obstacleObject->data.texture.constRect.y;

      res.y = obstacleY;
      break;
    }
  }
  return res;
}

// func will find shelter (either under a cloud or behind a rock)
static enum shelterType findShelter(App* app, int32_t* heightMap,
                                    Player* currPlayer,
                                    SDL_bool isFirstPlayer) {
  enum shelterType shelter;

  SDL_Point shelterPos = findNearestStone(isFirstPlayer);
  log_info("stone pos: %d (isFirst: %d)", shelterPos, isFirstPlayer);

  // if stone was not found
  if (shelterPos.x == -1) {
    shelterPos = findNearestCloud(isFirstPlayer);
    log_info("cloud pos: %d (isFirst: %d)", shelterPos, isFirstPlayer);

    if (shelterPos.x == -1) {
      return NONE;
    } else {
      shelter = CLOUD;
    }
  } else {
    shelter = STONE;
  }

  const int movingQuantum = 45;

  if (shelter == STONE) {
    //  if curr shelter is STONE we re just simply moving towards it
    int currDistance;
    if (isFirstPlayer) {
      currDistance =
          shelterPos.x - (app->currPlayer->tankObj->data.texture.constRect.x +
                          app->currPlayer->tankObj->data.texture.constRect.w);
    } else {
      currDistance =
          app->currPlayer->tankObj->data.texture.constRect.x - shelterPos.x;
    }
    while (
        currDistance >= movingQuantum / 2 &&
        !smoothMove(app, isFirstPlayer, isFirstPlayer, heightMap, obstacles)) {
      if (isFirstPlayer) {
        currDistance =
            shelterPos.x - (app->currPlayer->tankObj->data.texture.constRect.x +
                            app->currPlayer->tankObj->data.texture.constRect.w);
      } else {
        currDistance =
            app->currPlayer->tankObj->data.texture.constRect.x - shelterPos.x;
      }
    }
  } else if (shelter == CLOUD) {
    // If its a cloud, we try to hide beneath it a lil lefter(left player)
    // or righter(right player)
    // according to my super math calculations it will be about 120 px
    if (isFirstPlayer) {
      int playerShouldBeHereX = shelterPos.x - 120;
      int dx = app->currPlayer->tankObj->data.texture.constRect.x -
               playerShouldBeHereX;
      if (abs(dx) < movingQuantum) {
        return shelter;
      }
      SDL_bool isMovingRight = SDL_TRUE;

      if (dx < 0) {
        isMovingRight = SDL_FALSE;
      }

      dx = abs(dx);

      if (abs(dx) < movingQuantum) {
        return shelter;
      }

      if (!isMovingRight &&
          app->currPlayer->tankObj->data.texture.constRect.x <= 10) {
        return shelter;
      }

      // dx / movingQuantum = amount of steps required to move tank beneath the cloud
      for (int i = 0; i < (int)ceil(dx * 1. / movingQuantum); ++i) {
        if (smoothMove(app, isFirstPlayer, isMovingRight, heightMap,
                       obstacles)) {
          return shelter;
        }
      }
    } else {
      int playerShouldBeHereX = shelterPos.x + 30;
      int dx = playerShouldBeHereX -
               (app->currPlayer->tankObj->data.texture.constRect.x +
                currPlayer->tankObj->data.texture.constRect.w);

      if (abs(dx) < movingQuantum) {
        return shelter;
      }
      SDL_bool isMovingRight = SDL_FALSE;
      if (dx < 0) {
        isMovingRight = SDL_TRUE;
      }
      dx = abs(dx);

      if (isMovingRight &&
          app->currPlayer->tankObj->data.texture.constRect.x + 50 >=
              app->screenWidth) {
        return shelter;
      }

      log_warn("SECOND WANNA MOVE %d TIMES, right: %d", dx / movingQuantum,
               isMovingRight);

      for (int i = 0; i < (int)ceil(dx * 1. / movingQuantum); ++i) {
        if (smoothMove(app, isFirstPlayer, isMovingRight, heightMap,
                       obstacles)) {
          return shelter;
        }
      }
    }
  }
  return shelter;
}

static int decideLoop(App* app, Player* firstPlayer, Player* secondPlayer,
                      int32_t* heightMap, RenderObject* projectile,
                      RenderObject* explosion, SDL_bool* regenMap,
                      SDL_bool* recalcBulletPath, double initGunAngle,
                      int32_t maxPower, double windStrength,
                      SDL_Point* collisionP1, SDL_Point* collisionP2,
                      SDL_Point* collisionP3, int32_t collisionP1R,
                      int32_t collisionP2R, int32_t collisionP3R,
                      enum shootingPrio shootingPrio, double velMult) {
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

  for (int angle = 0; angle <= 120; ++angle) {
    double currAngle = app->currPlayer->tankGunObj->data.texture.angle;

    if (app->currPlayer == secondPlayer)
      currAngle += 180 + angle;
    else
      currAngle += -angle;

    currAngle = round(currAngle);
    currAngle = 360 - normalizeAngle(currAngle);

    for (int power = maxPower; power >= 0; --power) {
      int32_t hitPos = calcHitPosition(&currPos, power * velMult, initGunAngle,
                                       heightMap, app, collisionP1, collisionP2,
                                       collisionP3, collisionP1R, collisionP2R,
                                       collisionP3R, projectile, windStrength);
      log_fatal("%d %lf %lf", power, power * velMult, currAngle);
      // if weapon is broken the best option is to shoot obstacles near the enemy
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
        return 1;
      }
      // obstacle shoot
      if (hitPos == INT_MAX_VAL && shootingPrio != TANK) {
        smoothChangeAngle(app->currPlayer, angle, &app->currState,
                          recalcBulletPath);
        smoothChangePower(app->currPlayer, power, &app->currState,
                          recalcBulletPath);
        SDL_Delay(200);
        shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
              regenMap);
        recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
        recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
        return 1;
      }
    }
  }
  return 0;
}

void bot1Main(App* app, Player* firstPlayer, Player* secondPlayer,
              int32_t* heightMap, RenderObject* projectile,
              RenderObject* explosion, SDL_bool* regenMap,
              SDL_bool* recalcBulletPath, double initGunAngle) {
  Player* enemy;
  if (app->currPlayer == firstPlayer) {
    enemy = secondPlayer;
  } else {
    enemy = firstPlayer;
  }

  // getting collison for an enemy player
  SDL_Point collisionP1, collisionP2, collisionP3;
  int32_t collisionP1R, collisionP2R, collisionP3R;
  // getting player collisions centers and radiuses
  // player1 collision
  if (enemy == firstPlayer) {
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
  // player2 collison
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

  double velMultiplicator;
  int32_t explosionRadius;
  SDL_bool isHittableNearby;
  int32_t maxPower;
  switch (app->currWeapon) {
    // small bullet
    case 0:
      velMultiplicator = 2;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      maxPower = 50;
      break;
    // BIG BULLET
    case 1:
      velMultiplicator = 1.75;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      maxPower = 50;
      break;
    // small boom
    case 2:
      velMultiplicator = 1.25;
      explosionRadius = projectile->data.texture.constRect.w * 2;
      isHittableNearby = SDL_TRUE;
      maxPower = 75;
      break;
    // BIG BOOM
    case 3:
      velMultiplicator = 1.0;
      explosionRadius = projectile->data.texture.constRect.w * 4;
      isHittableNearby = SDL_TRUE;
      maxPower = 99;
      break;
    default:
      velMultiplicator = 1.0;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      maxPower = 99;
      break;
  }

  // getting current wind speed
  int32_t windStrengthMin, windStrengthMax;
  getWindRange(app, &windStrengthMin, &windStrengthMax);
  double windStrength = AVG(windStrengthMin, windStrengthMax);

  // 1. (!) firstly we should find shelter
  enum shelterType currShelterType = findShelter(
      app, heightMap, app->currPlayer, app->currPlayer == firstPlayer);

  if (decideLoop(app, firstPlayer, secondPlayer, heightMap, projectile,
                 explosion, regenMap, recalcBulletPath, initGunAngle, maxPower,
                 windStrength, &collisionP1, &collisionP2, &collisionP3,
                 collisionP1R, collisionP2R, collisionP3R, idgf,
                 velMultiplicator)) {
    return;
  }

  // that means we can move backwards(forwards) on right(left) tank
  if (currShelterType == STONE) {
    const int maxMovingAttempts = 2;
    //
    for (int i = 0; i < maxMovingAttempts; ++i) {
      smoothMove(app, app->currPlayer == firstPlayer,
                 app->currPlayer == secondPlayer, heightMap, obstacles);

      if (decideLoop(app, firstPlayer, secondPlayer, heightMap, projectile,
                     explosion, regenMap, recalcBulletPath, initGunAngle,
                     maxPower, windStrength, &collisionP1, &collisionP2,
                     &collisionP3, collisionP1R, collisionP2R, collisionP3R,
                     TANK, velMultiplicator)) {
        return;
      }
    }
    // IF HE WAS NOT ABLE TO HIT ENEMY STRAIGHT -> GO BACK BEHIND THE ROCK
    for (int i = 0; i < maxMovingAttempts; ++i) {
      smoothMove(app, app->currPlayer == firstPlayer,
                 app->currPlayer == firstPlayer, heightMap, obstacles);
      // hitting obstacle from safer position
      if (decideLoop(app, firstPlayer, secondPlayer, heightMap, projectile,
                     explosion, regenMap, recalcBulletPath, initGunAngle,
                     maxPower, windStrength, &collisionP1, &collisionP2,
                     &collisionP3, collisionP1R, collisionP2R, collisionP3R,
                     OBSTACLES, velMultiplicator)) {
        return;
      }
    }
  } else if (currShelterType == CLOUD) {
    const int maxMovingAttempts = 1;
    //
    for (int i = 0; i < maxMovingAttempts; ++i) {
      smoothMove(app, app->currPlayer == firstPlayer,
                 app->currPlayer == secondPlayer, heightMap, obstacles);

      if (decideLoop(app, firstPlayer, secondPlayer, heightMap, projectile,
                     explosion, regenMap, recalcBulletPath, initGunAngle,
                     maxPower, windStrength, &collisionP1, &collisionP2,
                     &collisionP3, collisionP1R, collisionP2R, collisionP3R,
                     TANK, velMultiplicator)) {
        return;
      }
    }
    // IF HE WAS NOT ABLE TO HIT ENEMY STRAIGHT -> GO BACK BEHIND THE ROCK
    for (int i = 0; i < maxMovingAttempts; ++i) {
      smoothMove(app, app->currPlayer == firstPlayer,
                 app->currPlayer == firstPlayer, heightMap, obstacles);
      // hitting obstacle from safer position
      if (decideLoop(app, firstPlayer, secondPlayer, heightMap, projectile,
                     explosion, regenMap, recalcBulletPath, initGunAngle,
                     maxPower, windStrength, &collisionP1, &collisionP2,
                     &collisionP3, collisionP1R, collisionP2R, collisionP3R,
                     OBSTACLES, velMultiplicator)) {
        return;
      }
    }
  }
}