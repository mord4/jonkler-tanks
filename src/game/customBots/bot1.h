#ifndef BOT1_H
#define BOT1_H

#define BOT1_ADDED

#include "../../App.h"

enum shelterType {
  NONE,
  STONE,
  CLOUD,
};

enum shootingPrio {
  idgf,
  TANK,
  OBSTACLES,
};

void bot1Main(App* app, Player* firstPlayer, Player* secondPlayer,
              int32_t* heightMap, RenderObject* projectile,
              RenderObject* explosion, SDL_bool* regenMap,
              SDL_bool* recalcBulletPath, double initGunAngle);

#endif