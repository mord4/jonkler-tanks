#ifndef GOTHGIRL_H
#define GOTHGIRL_H

#define BOT3_ADDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>

#include <float.h>
#include <math.h>

#include "../../math/math.h"
#include "../../math/rand.h"
#include "../obstacle.h"
#include "../player_movement.h"
#include "../specialConditions/wind.h"

void theGothGambit(App* app, Player* firstPlayer, Player* secondPlayer,
                   int32_t* heightMap, RenderObject* projectile,
                   RenderObject* explosion, SDL_bool* regenMap,
                   SDL_bool* recalcBulletPath, double initGunAngle);

#endif