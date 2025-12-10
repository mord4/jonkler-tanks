#include "bot3.h"

static Player* whoIsEnenmy(Player* currPlayer, Player* firstPlayer, Player* secondPlayer)
{
  if (currPlayer == firstPlayer) return secondPlayer;
  return firstPlayer;
}

static void setPlayerCollision(
  App* app, Player* enemy, Player* firstPlayer, Player* secondPlayer,
  SDL_Point* collisionP1, SDL_Point* collisionP2, SDL_Point* collisionP3,
  int32_t* collisionP1R, int32_t* collisionP2R, int32_t* collisionP3R
)
{
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

static void setWeaponStats(
  int32_t currWeapon, RenderObject* projectile,
  double* velMultiplicator, int32_t* explosionRadius, SDL_bool* isHittableNearby,
  int32_t* maxPower
)
{
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

// func will find shelter (either under a cloud or behind a rock)
static void tryFindShelter(App* app, int32_t* heightMap, SDL_bool isFirstPlayer) {
  SDL_Point shelterPos = findNearestStone(isFirstPlayer);
  if (app->currPlayer->movesLeft == 0 || shelterPos.x == -1) return;

  const int movingQuantum = 45;

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
}

static int32_t countCloudsRightNow() {
  int32_t count = MAXCLOUDS;
  for (int32_t i = MAXCLOUDS + MAXSTONES - 1; i >= MAXSTONES; --i) {
    if (obstacles[i].obstacleObject == NULL || obstacles[i].health == 0) {
      count--;
    }
  }
  return count;
}

void theGothGambit(
  App* app,
  Player* firstPlayer,
  Player* secondPlayer,
  int32_t* heightMap,
  RenderObject* projectile,
  RenderObject* explosion,
  SDL_bool* regenMap,
  SDL_bool* recalcBulletPath,
  double initGunAngle
)
{
  if (!countCloudsRightNow()) tryFindShelter(app, heightMap, app->currPlayer == firstPlayer);

  Player* enemy = whoIsEnenmy(app->currPlayer, firstPlayer, secondPlayer);

  SDL_Point collisionP1, collisionP2, collisionP3;
  int32_t collisionP1R, collisionP2R, collisionP3R;

  setPlayerCollision(
    app, enemy, firstPlayer, secondPlayer,
    &collisionP1, &collisionP2, &collisionP3,
    &collisionP1R, &collisionP2R, &collisionP3R
  );

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

  int32_t windStrengthMin, windStrengthMax;
  getWindRange(app, &windStrengthMin, &windStrengthMax);

  double velMultiplicator;
  int32_t explosionRadius;
  SDL_bool isHittableNearby;
  int32_t maxPower;

  setWeaponStats(
    app->currWeapon, projectile,
    &velMultiplicator, &explosionRadius, &isHittableNearby, &maxPower
  );

  for (int32_t angle = 120; angle >= 0; --angle) {
    double currAngle = app->currPlayer->tankGunObj->data.texture.angle;

    if (app->currPlayer == secondPlayer) currAngle += 180 + angle;
    else currAngle += -angle;

    currAngle = round(currAngle);
    currAngle = 360 - normalizeAngle(currAngle);

    for (int32_t power = 0; power <= maxPower; ++power) {
      int32_t hitPos = calcHitPosition(
          &currPos, power * velMultiplicator, currAngle, heightMap, app,
          &collisionP1, &collisionP2, &collisionP3, collisionP1R, collisionP2R,
          collisionP3R, projectile, AVG(windStrengthMin, windStrengthMax));
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
        return;
      }
      
    }
  }
  smoothChangeAngle(app->currPlayer, app->currPlayer->gunAngle, &app->currState,
                    recalcBulletPath);
  smoothChangePower(app->currPlayer, app->currPlayer->firingPower, &app->currState,
                    recalcBulletPath);
  SDL_Delay(200);
  shoot(app, firstPlayer, secondPlayer, projectile, explosion, heightMap,
        regenMap);
  recalcPlayerPos(app, firstPlayer, heightMap, 0, 5);
  recalcPlayerPos(app, secondPlayer, heightMap, 0, 8);
  return;
}
