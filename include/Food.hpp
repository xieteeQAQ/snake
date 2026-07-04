#pragma once

#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

struct Food
{
    Food() = default;

    SDL_Texture *_tex;
    SDL_FRect _src{.x = 0, .y = 0, .w = 900, .h = 900};
    SDL_FRect _dst{.x = 0, .y = 0, .w = 50, .h = 50};
};
