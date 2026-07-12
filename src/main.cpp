#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <thread>
#include <chrono>
#include "State.hpp"
#include "Food.hpp"
#include "snake.hpp"
#include "gaobject.hpp"

// int fps = 60;
bool debug = false;

int main(int argc, char **argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL: init failed\n";
        return 1;
    }

    State state;
    state.init();

    Resources res;
    res.load(state);

    GameState gs(state);
    createMap(state, gs, res);
    uint64_t prevTime = SDL_GetTicks();

    bool running = true;
    while (running)
    {
        uint64_t nowTime = SDL_GetTicks();
        float deltaTime = (nowTime - prevTime) / 1000.0f;
        prevTime = nowTime;

        SDL_Event event{0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            {
                running = false;
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
                state.width = event.window.data1;
                state.height = event.window.data2;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                handleKayInput(state, gs, gs.player(), deltaTime, event.key.scancode, true);
                break;
            }
            case SDL_EVENT_KEY_UP:
            {
                handleKayInput(state, gs, gs.player(), deltaTime, event.key.scancode, false);
                break;
            }
            }
        }
        SDL_SetRenderDrawColor(state._renderer, 30, 30, 30, 255);
        SDL_RenderClear(state._renderer);
        drawBackground(state, gs, gs.player(), res.background);
        generateFood(state, gs, res, deltaTime);

        for (auto &layer : gs.layers)
        {
            for (GameObject &obj : layer)
            {
                update(state, gs, res, obj, deltaTime);
            }
        }

        for (auto &layer : gs.layers)
        {
            for (GameObject &obj : layer)
            {
                drawObject(state, gs, obj, deltaTime);
            }
        }

        gs.mapViewport.x = (gs.player().position.x + 64 / 2) - gs.mapViewport.w / 2;
        gs.mapViewport.y = (gs.player().position.y + 64 / 2) - gs.mapViewport.h / 2;

        if (debug)
        {
            writeDebugText(state, gs, deltaTime);
        }

        SDL_RenderPresent(state._renderer);
        // Uint32 delay = (1 / static_cast<float>(fps)) * 1000.0f;
        // SDL_Delay(delay);
    }

    std::cout << "hello world!\n";

    res.unload();
    state.~State();
    SDL_Quit();
    return 0;
}