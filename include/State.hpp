#pragma once

#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

struct State
{
    State();
    ~State();

    void init();
    void check_init();

    SDL_Window *_window;
    SDL_Renderer *_renderer;
    MIX_Mixer *_mixer;
    TTF_TextEngine *_engine;
    int width = 800;
    int height = 600;
    int logW = 640;
    int logH = 400;
    float logX = 0;
    float logY = 0;
    const bool *keys;
};
