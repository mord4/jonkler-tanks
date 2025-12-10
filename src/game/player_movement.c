#include "player_movement.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>

#include "specialConditions/spread.h"

#include "../math/math.h"
#include "autosave.h"
#include "bot.h"
#include "log/log.h"
#include "obstacle.h"

// function fires a shot from currPlayer and detects any hits to enemyPlayer
void shoot(App* app, Player* firstPlayer, Player* secondPlayer,
           RenderObject* projectile, RenderObject* explosion,
           int32_t* heightMap, SDL_bool* regenMap) {
  if (app->currState != PLAY) return;

  Mix_PlayChannel(-1, app->sounds[0], 0);

  // determining which player is the current enemy
  Player* enemyPlayer;
  if (app->currPlayer == firstPlayer) {
    enemyPlayer = secondPlayer;
  } else {
    enemyPlayer = firstPlayer;
  }

  // that angle is clockwise
  double currGunAngle = app->currPlayer->tankGunObj->data.texture.angle;

  // calculating angle specifically for a current player
  if (app->currPlayer == secondPlayer) {
    currGunAngle += 180 - app->currPlayer->tankGunObj->data.texture.angleAlt;
  } else {
    currGunAngle += app->currPlayer->tankGunObj->data.texture.angleAlt;
  }

  // normalizing just to be sure its in [0;2pi) and now its counterclockwise
  currGunAngle = 360 - normalizeAngle(currGunAngle);

  // add spread to current angle
  if (app->currPlayer->buffs.weaponIsBroken) {
    currGunAngle = addSpreadToCurrAngle(app->currWeapon, currGunAngle);
  }

  // thats an init pos of a curr player tank
  SDL_Point tempPos;
  if (app->currPlayer == firstPlayer) {
    tempPos = getPixelScreenPosition(
        (SDL_Point){
            app->currPlayer->tankObj->data.texture.scaleRect.x,
            app->currPlayer->tankObj->data.texture.scaleRect.y,
        },
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        app->currPlayer->tankObj->data.texture.angle,
        (SDL_Point){27 * app->scalingFactorX, 7 * app->scalingFactorY});
  } else {
    tempPos = getPixelScreenPosition(
        (SDL_Point){
            app->currPlayer->tankObj->data.texture.scaleRect.x,
            app->currPlayer->tankObj->data.texture.scaleRect.y,
        },
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        app->currPlayer->tankObj->data.texture.angle,
        (SDL_Point){16 * app->scalingFactorX, 7 * app->scalingFactorY});
  }

  SDL_FPoint initPos = {
      .x = tempPos.x,
      .y = tempPos.y,
  };

  SDL_FPoint relativePos = {
      .x = 0.0f,
      .y = 0.0f,
  };

  SDL_Point savedPos = {
      .x = (int32_t)initPos.x,
      .y = (int32_t)initPos.y,
  };

  initPos.x /= app->scalingFactorX;
  initPos.y /= app->scalingFactorY;

  // initpos now points to the edge of the tank gun
  initPos.x += 20 * cos(DEGTORAD(currGunAngle));
  initPos.y -= 20 * sin(DEGTORAD(currGunAngle));

  // printf("initpos: x: %lf y: %lf, angle: %lf\n", initPos.x, initPos.y, currGunAngle);
  //

  // showing projectile
  projectile->disableRendering = SDL_FALSE;

  // select the time interval for recalculations
  const double dt = 1. / 45;
  double currTime = 0.0;

  double vel;
  int32_t explosionRadius;
  SDL_bool isHittableNearby;
  double damageMultiplier;
  int32_t healthDamage;
  switch (app->currWeapon) {
    // small bullet
    case 0:
      vel = app->currPlayer->firingPower * 2;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      damageMultiplier = 1.5;
      healthDamage = 5;
      break;
    // BIG BULLET
    case 1:
      vel = app->currPlayer->firingPower * 1.75;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      damageMultiplier = 2.0;
      healthDamage = 10;
      break;
    // small boom
    case 2:
      vel = app->currPlayer->firingPower * 1.25;
      explosionRadius = projectile->data.texture.constRect.w * 2;
      isHittableNearby = SDL_TRUE;
      damageMultiplier = 1.25;
      healthDamage = 8;
      break;
    // BIG BOOM
    case 3:
      vel = app->currPlayer->firingPower * 1;
      explosionRadius = projectile->data.texture.constRect.w * 4;
      isHittableNearby = SDL_TRUE;
      damageMultiplier = 1.75;
      healthDamage = 16;
      break;
    default:
      vel = app->currPlayer->firingPower;
      explosionRadius = projectile->data.texture.constRect.w;
      isHittableNearby = SDL_FALSE;
      damageMultiplier = 1;
      healthDamage = 1;
      break;
  }

  // printf("initVel:%lf\n", vel);
  //

  // player collision center and radiuses
  SDL_Point collisionP1, collisionP2, collisionP3;
  int32_t collisionP1R, collisionP2R, collisionP3R;
  // getting player collisions centers
  if (app->currPlayer == secondPlayer) {
    collisionP1 = getPixelScreenPosition(
        (SDL_Point){enemyPlayer->tankObj->data.texture.scaleRect.x,
                    enemyPlayer->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemyPlayer->tankObj->data.texture.angle,
        (SDL_Point){35 * app->scalingFactorX, 19 * app->scalingFactorY});
    collisionP2 = getPixelScreenPosition(
        (SDL_Point){enemyPlayer->tankObj->data.texture.scaleRect.x,
                    enemyPlayer->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemyPlayer->tankObj->data.texture.angle,
        (SDL_Point){20 * app->scalingFactorX, 12 * app->scalingFactorY});
    collisionP3 = getPixelScreenPosition(
        (SDL_Point){enemyPlayer->tankObj->data.texture.scaleRect.x,
                    enemyPlayer->tankObj->data.texture.scaleRect.y},
        (SDL_Point){5 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemyPlayer->tankObj->data.texture.angle,
        (SDL_Point){7 * app->scalingFactorX, 19 * app->scalingFactorY});
    // radiuses
    collisionP1R = 9 * hypot(app->scalingFactorX, app->scalingFactorY);
    collisionP2R = 13 * hypot(app->scalingFactorX, app->scalingFactorY);
    collisionP3R = 10 * hypot(app->scalingFactorX, app->scalingFactorY);
  }
  // right player collison
  else {
    collisionP1 = getPixelScreenPosition(
        (SDL_Point){enemyPlayer->tankObj->data.texture.scaleRect.x,
                    enemyPlayer->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemyPlayer->tankObj->data.texture.angle,
        (SDL_Point){11 * app->scalingFactorX, 19 * app->scalingFactorY});
    collisionP2 = getPixelScreenPosition(
        (SDL_Point){enemyPlayer->tankObj->data.texture.scaleRect.x,
                    enemyPlayer->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemyPlayer->tankObj->data.texture.angle,
        (SDL_Point){23 * app->scalingFactorX, 13 * app->scalingFactorY});
    collisionP3 = getPixelScreenPosition(
        (SDL_Point){enemyPlayer->tankObj->data.texture.scaleRect.x,
                    enemyPlayer->tankObj->data.texture.scaleRect.y},
        (SDL_Point){8 * app->scalingFactorX, 27 * app->scalingFactorY},
        enemyPlayer->tankObj->data.texture.angle,
        (SDL_Point){36 * app->scalingFactorX, 18 * app->scalingFactorY});
    // radiuses
    collisionP1R = 11 * hypot(app->scalingFactorX, app->scalingFactorY);
    collisionP2R = 11 * hypot(app->scalingFactorX, app->scalingFactorY);
    collisionP3R = 9 * hypot(app->scalingFactorX, app->scalingFactorY);
  }

  double windAngleRad = DEGTORAD(normalizeAngle(
      360 - app->globalConditions.wind.directionIcon->data.texture.angle));
  double windStrength = app->globalConditions.wind.windStrength;
  double windStrengthX = windStrength * cos(windAngleRad);
  double windStrengthY = windStrength * sin(windAngleRad);

  double vx = vel * cos(DEGTORAD(currGunAngle));
  double vy = vel * sin(DEGTORAD(currGunAngle));

  // main shooting loop
  while (app->currState == PLAY) {
    currTime += dt;

    // getting coords with respect to the init pos
    getPositionAtSpecTime(&relativePos, vx, vy, windStrengthX, windStrengthY,
                          currTime);

    int32_t currX = initPos.x + relativePos.x;
    int32_t currY = initPos.y - relativePos.y;
    // offset for left tank
    if (app->currPlayer == firstPlayer) {
      currY -= 10;
    }

    // calculating angle between the old projectile pos and the new 1
    double currAngle =
        360 - RADTODEG(atan2(savedPos.y - currY, currX - savedPos.x));

    // printf("angle: %lf\n", currAngle);
    //

    // now this is a current projectile position
    savedPos = (SDL_Point){
        .x = currX,
        .y = currY,
    };

    int32_t currMapHeight =
        app->screenHeight -
        heightMap[(int32_t)round(currX * app->scalingFactorX)];

    //printf("cmh:%d, (%d %d)\n", currMapHeight, currX, currY);

    projectile->data.texture.constRect.x = currX;
    projectile->data.texture.constRect.y = currY;
    projectile->data.texture.angle = currAngle;

    int32_t currXScaled = currX * app->scalingFactorX;
    int32_t currYScaled = currY * app->scalingFactorY;

    // out of bounds check
    if (currX < 0 || currXScaled >= app->screenWidth ||
        currYScaled > app->screenHeight) {
      //     printf("out of bounds!\n");
      //
      break;
    }

    // firsty checking the collision
    if (checkObstacleCollisions(currX, currY)) {
      explosion->data.texture.constRect.h = explosionRadius;
      explosion->data.texture.constRect.w = explosionRadius;
      explosion->data.texture.constRect.x =
          currX - explosion->data.texture.constRect.w / 2;
      explosion->data.texture.constRect.y =
          currY - explosion->data.texture.constRect.h / 2;
      explosion->data.texture.currFrame = 0;
      explosion->disableRendering = SDL_FALSE;
      break;
    }

    app->wasHitten = false;
    // if we hit enemy straight
    if (isInCircle(currXScaled, currYScaled, &collisionP1, collisionP1R) ||
        isInCircle(currXScaled, currYScaled, &collisionP2, collisionP2R) ||
        isInCircle(currXScaled, currYScaled, &collisionP3, collisionP3R)) {
      //     printf("COLLISION HIT!\n");
      //
      app->wasHitten = true;
      Mix_PlayChannel(-1, app->sounds[2], 0);

      int32_t enemyCenter = enemyPlayer->tankObj->data.texture.constRect.x +
                            enemyPlayer->tankObj->data.texture.constRect.w / 2.;
      // hitting father than center
      if (currX > enemyCenter) {
        double deltaDamage =
            10 + ((enemyCenter +
                   enemyPlayer->tankObj->data.texture.constRect.w / 2.) -
                  currX) *
                     damageMultiplier;
        if (app->currPlayer->buffs.isDoubleDamage) {
          deltaDamage *= 2.0;
          healthDamage *= 2.0;
        }
        if (enemyPlayer->buffs.isShielded) {
          deltaDamage *= 0.15;  // 85% damage blocked
          healthDamage *= 0.15;
        }

        enemyPlayer->health -= healthDamage;
        app->currPlayer->score += (int32_t)deltaDamage;
      }
      // hitting before the center
      else {
        double deltaDamage =
            10 + (currX - enemyPlayer->tankObj->data.texture.constRect.x) *
                     damageMultiplier;
        if (app->currPlayer->buffs.isDoubleDamage) {
          deltaDamage *= 2.0;
          healthDamage *= 2.0;
        }
        if (enemyPlayer->buffs.isShielded) {
          deltaDamage *= 0.15;  // 85% damage blocked
          healthDamage *= 0.15;
        }

        enemyPlayer->health -= healthDamage;
        app->currPlayer->score += (int32_t)deltaDamage;
      }
      if (isHittableNearby == SDL_FALSE) {
        explosion->data.texture.constRect.h =
            projectile->data.texture.constRect.h;
        explosion->data.texture.constRect.w =
            projectile->data.texture.constRect.w;
      } else {
        explosion->data.texture.constRect.h = explosionRadius;
        explosion->data.texture.constRect.w = explosionRadius;
      }
      explosion->data.texture.constRect.x =
          currX - explosion->data.texture.constRect.w / 2;
      explosion->data.texture.constRect.y =
          currY - explosion->data.texture.constRect.h / 2;
      explosion->data.texture.currFrame = 0;
      explosion->disableRendering = SDL_FALSE;

      break;
    }

    // ground impact check
    if ((currY + projectile->data.texture.constRect.h) * app->scalingFactorY >=
        currMapHeight) {
      log_warn("GROUND HIT at %d", currX);

      if (isHittableNearby) {
        int32_t center = enemyPlayer->tankObj->data.texture.constRect.x +
                         enemyPlayer->tankObj->data.texture.constRect.w / 2;

        // hit near to enemy, so shot in [leftTankCorner - explRadius; rightTankCorner + explRadius];
        if (currX <= center + explosionRadius +
                         enemyPlayer->tankObj->data.texture.constRect.w / 2 &&
            currX >= center - explosionRadius -
                         enemyPlayer->tankObj->data.texture.constRect.w / 2) {
          // hitting after center (after right tank corner)
          if (currX > center) {
            double deltaDamage =
                5 +
                ((center + enemyPlayer->tankObj->data.texture.constRect.w / 2. +
                  explosionRadius) -
                 currX) *
                    damageMultiplier / 1.5;
            if (app->currPlayer->buffs.isDoubleDamage) {
              deltaDamage *= 2.0;
              healthDamage *= 2.0 / 2;
            }
            if (enemyPlayer->buffs.isShielded) {
              deltaDamage *= 0.15;  // 85% damage blocked
              healthDamage *= 0.15 / 2;
            }

            enemyPlayer->health -= healthDamage;
            app->currPlayer->score += (int32_t)deltaDamage;
          }
          // hitting before center (before left tank corner)
          else {
            double deltaDamage =
                5 + (currX - (enemyPlayer->tankObj->data.texture.constRect.x -
                              explosionRadius)) *
                        damageMultiplier / 1.5;
            if (app->currPlayer->buffs.isDoubleDamage) {
              deltaDamage *= 2.0;
              healthDamage *= 2.0 / 2;
            }
            if (enemyPlayer->buffs.isShielded) {
              deltaDamage *= 0.15;  // 85% damage blocked
              healthDamage *= 0.15 / 2;
            }

            enemyPlayer->health -= healthDamage;
            app->currPlayer->score += (int32_t)deltaDamage;
          }
        }

        int32_t explosionCenter = currX * app->scalingFactorX;

        for (int32_t i = 1; i <= explosionRadius; ++i) {
          int32_t delta = sqrt(explosionRadius * explosionRadius - i * i);
          if (explosionCenter - i >= 0) {
            heightMap[explosionCenter - i] -= delta;
          }
          if (explosionCenter + i < app->screenWidth) {
            heightMap[explosionCenter + i] -= delta;
          }
        }

        heightMap[explosionCenter] -= explosionRadius;

        *regenMap = SDL_TRUE;

        recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
        recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
      }
      // 'respawning' explosion texture
      explosion->data.texture.constRect.h = explosionRadius;
      explosion->data.texture.constRect.w = explosionRadius;
      explosion->data.texture.constRect.x =
          currX - explosion->data.texture.constRect.w / 2;
      explosion->data.texture.constRect.y =
          currY - explosion->data.texture.constRect.h / 2;
      explosion->data.texture.currFrame = 0;
      explosion->disableRendering = SDL_FALSE;
      break;
    }

    SDL_Delay(3);
  }

  // hiding it again
  projectile->disableRendering = SDL_TRUE;
}

void recalcPlayerPos(App* app, Player* player, int32_t* heightMap, int32_t dx,
                     int32_t xOffset) {
  RenderObject* tankObj = player->tankObj;
  RenderObject* gunObj = player->tankGunObj;

  double newAngle =
      360 - getAngle((tankObj->data.texture.constRect.x + xOffset + dx) *
                         app->scalingFactorX,
                     heightMap, 20 * app->scalingFactorX);

  tankObj->data.texture.constRect.x += dx;
  tankObj->data.texture.constRect.y =
      -27 + app->screenHeight / app->scalingFactorY -
      heightMap[(int32_t)((tankObj->data.texture.constRect.x + 5) *
                          app->scalingFactorX)] /
          app->scalingFactorY;
  tankObj->data.texture.angle = newAngle;

  gunObj->data.texture.constRect.x += dx;
  gunObj->data.texture.constRect.y =
      -27 + app->screenHeight / app->scalingFactorY -
      heightMap[(int32_t)((gunObj->data.texture.constRect.x + 5) *
                          app->scalingFactorX)] /
          app->scalingFactorY;
  gunObj->data.texture.angle = newAngle;

  player->tankAngle = newAngle;
}

void recalcPlayerGunAngle(Player* player, int32_t dy) {
  player->gunAngle += dy;
  player->tankGunObj->data.texture.angleAlt = -player->gunAngle;
}

int32_t playerMove(void* data) {
  log_info("started player move thread! wazup o//");

  struct UpdateConditions {
    SDL_bool updateWind;
  };

  struct paramsStruct {
    App* app;
    Player* firstPlayer;
    Player* secondPlayer;
    int32_t* heightMap;
    RenderObject* projectile;
    RenderObject* explosion;
    SDL_bool* regenMap;
    SDL_bool* recalcBulletPath;
    SDL_bool* hideBulletPath;
    SDL_bool* isSpinning;
    struct UpdateConditions* updateConditions;
    uint32_t mapSeed;
  };

  struct paramsStruct* params = (struct paramsStruct*)data;

  App* app = params->app;
  Player* firstPlayer = params->firstPlayer;
  Player* secondPlayer = params->secondPlayer;
  int32_t* heightMap = params->heightMap;
  RenderObject* projectile = params->projectile;
  RenderObject* explosion = params->explosion;
  SDL_bool* regenMap = params->regenMap;
  SDL_bool* recalcBulletPath = params->recalcBulletPath;
  SDL_bool* hideBulletPath = params->hideBulletPath;
  SDL_bool* isSpinning = params->isSpinning;
  uint32_t mapSeed = params->mapSeed;
  struct UpdateConditions* updateConditions = params->updateConditions;

  while (app->currState == PLAY) {
    SDL_Delay(16);
    // skipping if player is in animation
    if (app->currPlayer->inAnimation) continue;
    // blocking input during event spin
    if (*isSpinning) continue;
    // each player shot 12 times => game is over rn
    if (firstPlayer->health <= 0 || secondPlayer->health <= 0) {
      SDL_Delay(1500);
      if (firstPlayer->score > secondPlayer->score) {
        if (firstPlayer->type != MONKE) {
          app->winner = 3;
        } else {
          app->winner = 1;
          app->winnerScore = firstPlayer->score;
        }
      } else if (firstPlayer->score < secondPlayer->score) {
        if (secondPlayer->type != MONKE) {
          app->winner = 3;
        } else {
          app->winner = 2;
          app->winnerScore = secondPlayer->score;
        }
      } else {
        app->winner = 0;
      }
      app->currState = LEADERBOARD_ADD;
      continue;
    }
    // if currPlayer played by a user
    if (app->currPlayer->type == MONKE) {
      // if weapon wasn't chosen
      while (app->currWeapon == -1) {
        SDL_Delay(16);
      }
      // moving right
      if (app->keyStateArr[SDL_SCANCODE_RIGHT] && app->currPlayer->movesLeft) {
        // now in animation
        app->currPlayer->inAnimation = SDL_TRUE;
        *hideBulletPath = SDL_TRUE;
        smoothMove(app, app->currPlayer == firstPlayer, SDL_TRUE, heightMap,
                   obstacles);
        *hideBulletPath = SDL_FALSE;
        saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                         app->currPlayer == firstPlayer, mapSeed);
        *recalcBulletPath = SDL_TRUE;
        // leaving animation
        app->currPlayer->inAnimation = SDL_FALSE;
      }

      // moving left
      else if (app->keyStateArr[SDL_SCANCODE_LEFT] &&
               app->currPlayer->movesLeft) {
        // now in animation
        app->currPlayer->inAnimation = SDL_TRUE;
        *hideBulletPath = SDL_TRUE;
        smoothMove(app, app->currPlayer == firstPlayer, SDL_FALSE, heightMap,
                   obstacles);
        *hideBulletPath = SDL_FALSE;
        saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                         app->currPlayer == firstPlayer, mapSeed);
        *recalcBulletPath = SDL_TRUE;
        // leaving animation
        app->currPlayer->inAnimation = SDL_FALSE;
      }

      // gun moving up
      else if (app->keyStateArr[SDL_SCANCODE_UP]) {
        while (app->keyStateArr[SDL_SCANCODE_UP]) {
          SDL_Delay(30);
          // if angle is already maxed
          if (fabs((app->currPlayer->gunAngle - 120.0)) < 1e-9) {
            break;
          }
          recalcPlayerGunAngle(app->currPlayer, 1);
          *recalcBulletPath = SDL_TRUE;
        }
        saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                         app->currPlayer == firstPlayer, mapSeed);
      }

      // gun moving down
      else if (app->keyStateArr[SDL_SCANCODE_DOWN]) {
        while (app->keyStateArr[SDL_SCANCODE_DOWN]) {
          SDL_Delay(30);
          // if angle is already maxed
          if (fabs(app->currPlayer->gunAngle) < 1e-9) {
            app->currPlayer->gunAngle = 0.0;
            break;
          }
          recalcPlayerGunAngle(app->currPlayer, -1);
          *recalcBulletPath = SDL_TRUE;
        }
        saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                         app->currPlayer == firstPlayer, mapSeed);
      }

      // decreasing power
      else if (app->keyStateArr[SDL_SCANCODE_S]) {
        while (app->keyStateArr[SDL_SCANCODE_S]) {
          SDL_Delay(30);
          // if angle is already maxed
          if (app->currPlayer->firingPower <= 1) {
            app->currPlayer->firingPower = 1;
            break;
          }
          app->currPlayer->firingPower--;
          *recalcBulletPath = SDL_TRUE;
        }
        saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                         app->currPlayer == firstPlayer, mapSeed);
      }

      // increasing power
      else if (app->keyStateArr[SDL_SCANCODE_W]) {
        while (app->keyStateArr[SDL_SCANCODE_W]) {
          SDL_Delay(30);
          // if angle is already maxed
          if (app->currPlayer->firingPower >= 99) {
            app->currPlayer->firingPower = 99;
            break;
          }
          app->currPlayer->firingPower++;
          *recalcBulletPath = SDL_TRUE;
        }
        saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                         app->currPlayer == firstPlayer, mapSeed);
      }

      // should be firing when pressing space
      else if (app->keyStateArr[SDL_SCANCODE_SPACE]) {
        // starting animation state
        app->currPlayer->inAnimation = SDL_TRUE;

        shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
              regenMap);

        // stopping animation state
        app->currPlayer->inAnimation = SDL_FALSE;

        if (app->currPlayer == firstPlayer) {
          app->currPlayer = secondPlayer;
        } else {
          app->currPlayer = firstPlayer;
        }

        app->timesPlayed++;
        app->currWeapon = -1;
        saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                         app->currPlayer == firstPlayer, mapSeed);

        log_info("players swapped");
        updateConditions->updateWind = SDL_TRUE;
        *recalcBulletPath = SDL_TRUE;
        SDL_Delay(200);
      }
    }
    // if played by bot
    else {
      app->currPlayer->inAnimation = SDL_TRUE;
      // if weapon wasn't chosen
      while (app->currWeapon == -1) {
        SDL_Delay(16);
      }
      log_info("currweapon - %d", app->currWeapon);
      // !! FULLY DISABLING BULLET PATH RENDERING FOR BOTS
      // !! JUST FOR NOW
      *hideBulletPath = SDL_TRUE;
      botMain(app, firstPlayer, secondPlayer, heightMap, projectile, explosion,
              regenMap, recalcBulletPath, app->currPlayer->type);
      *hideBulletPath = SDL_FALSE;
      app->currPlayer->inAnimation = SDL_FALSE;

      // switching players
      if (app->currPlayer == firstPlayer) {
        app->currPlayer = secondPlayer;
      } else {
        app->currPlayer = firstPlayer;
      }
      recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
      recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);

      app->timesPlayed++;
      app->currWeapon = -1;
      saveCurrentState(app, firstPlayer, secondPlayer, heightMap,
                       app->currPlayer == firstPlayer, mapSeed);

      updateConditions->updateWind = SDL_TRUE;
      *recalcBulletPath = SDL_TRUE;
    }
  }

  log_info("[bot] killed player move thread! byee o//");

  return 0;
}
