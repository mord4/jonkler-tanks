#ifndef BOT2_H
#define BOT2_H

#define BOT2_ADDED
#include "../../App.h"

void bot2Main(App* app, Player* firstPlayer, Player* secondPlayer,
              int32_t* heightMap, RenderObject* projectile,
              RenderObject* explosion, SDL_bool* regenMap,
              SDL_bool* recalcBulletPath, double initGunAngle);

#endif
