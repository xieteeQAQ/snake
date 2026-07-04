#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "State.hpp"
#include "Food.hpp"
#include "snake.hpp"
#include "gaobject.hpp"

int fps = 60;

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
    res.loadTex(state._renderer, "image/player_normal.png");

    GameState gs;
    GameObject player;
    player.setType(ObjectType::player);
    player.tex = res._tex;
    player.acceleration = glm::vec2(200, 200);
    player.maxSpeedX = 150;
    player.maxSpeedY = 150;
    player.position.x = state.logX / 2.0f - 32.0f;
    player.position.y = state.logY / 2.0f - 32.0f;
    gs.layers[LAYER_IDX_CHARACTERS].push_back(player);

    uint64_t prevTime = SDL_GetTicks();
    bool running = true;
    while (running)
    {
        uint64_t nowTime = SDL_GetTicks();
        float deltaTime = (nowTime - prevTime) / 1000.0f;
        prevTime = nowTime;

        if (deltaTime > 0.05f)
            deltaTime = 0.05f;

        SDL_Event event{0};
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                state.width = event.window.data1;
                state.height = event.window.data2;
            }
        }
        SDL_SetRenderDrawColor(state._renderer, 255, 255, 255, 255);
        SDL_RenderClear(state._renderer);

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

        SDL_RenderPresent(state._renderer);
        SDL_Delay(1 / fps * 1000);
    }

    std::cout << "hello world!\n";

    res.unload();
    state.~State();
    SDL_Quit();
    return 0;
}