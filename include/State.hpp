#pragma once

#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

struct State
{
    State();
    ~State();

    void init();
    void check_init();

    SDL_Window *_window;
    SDL_Renderer *_renderer;
    int width = 800;
    int height = 600;
    int logW = 640;
    int logH = 400;
    float logX = 0;
    float logY = 0;
    const bool *keys;
};
