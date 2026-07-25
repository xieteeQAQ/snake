#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <thread>
#include <chrono>
#include "State.hpp"
#include "snake.hpp"
#include "gaobject.hpp"

const int FPS = 60;
const int frameDelay = 1000 / FPS;
float playtime = 0;
bool debug = false;
bool collision_box = false;
std::random_device rd;

int main(int argc, char **argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return 1;
    }

    if (!MIX_Init())
    {
        SDL_Log("MIX init failed: %s", SDL_GetError());
        return 1;
    }

    if (!TTF_Init())
    {
        SDL_Log("TTF init failed: %s", SDL_GetError());
    }

    State state;
    state.init();
    SDL_SetRenderVSync(state._renderer, 1);

    Resources res;
    res.load(state);

    GameState gs(state);
    createMap(state, gs, res);
    uint64_t prevTime = SDL_GetTicks();

    BulletGenerater BG_circleStickyRice(1, 1, res.warning, BulletGenerater::Type::circleStickyRice);
    gs.bulletGeneraters.push_back(BG_circleStickyRice);

    bool running = true;
    playBGM(res.Graze_The_Roof);
    while (running)
    {
        uint64_t nowTime = SDL_GetTicks();
        float deltaTime = (nowTime - prevTime) / 1000.0f;
        prevTime = nowTime;
        playtime += deltaTime;

        SDL_SetRenderDrawColor(state._renderer, 30, 30, 30, 255);
        SDL_RenderClear(state._renderer);
        drawBackground(state, gs, gs.player(), res.background);

        SDL_Event event{0};
        if (gs.player().data.player.currentHealth != 0)
        {
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
                    handleKeyInput(state, gs, gs.player(), res, deltaTime, event.key.scancode, true);
                    break;
                }
                case SDL_EVENT_KEY_UP:
                {
                    handleKeyInput(state, gs, gs.player(), res, deltaTime, event.key.scancode, false);
                    break;
                }
                }
            }

            generateFood(state, gs, res, deltaTime);
            generatePotatoMine(state, gs, res, deltaTime);
            updateBulletGenerater(state, gs, res, deltaTime);

            for (int i = 0; i < gs.timers.size(); ++i)
            {
                if (!gs.timers[i].isTimeout())
                {
                    gs.timers[i].step(deltaTime);
                }
            }

            for (int i = 0; i < gs.layers.size(); ++i)
            {
                for (int j = 0; j < gs.layers[i].size(); ++j)
                {
                    update(state, gs, res, gs.layers[i][j], deltaTime);
                    if (gs.layers[i][j].currentAnimation != -1)
                    {
                        gs.layers[i][j].animation[gs.layers[i][j].currentAnimation].step(deltaTime);
                    }
                }
            }
            for (int i = 0; i < gs.bullets.size(); ++i)
            {
                for (int j = 0; j < gs.bullets[i].size(); ++j)
                {
                    update(state, gs, res, gs.bullets[i][j], deltaTime);
                    if (gs.bullets[i][j].currentAnimation != -1)
                    {
                        gs.bullets[i][j].animation[gs.bullets[i][j].currentAnimation].step(deltaTime);
                    }
                }
                for (int i = BULLET_IDX_FRYING; i < gs.bullets.size(); ++i)
                {
                    gs.bullets[i].erase(std::remove_if(gs.bullets[i].begin(), gs.bullets[i].end(), [](GameObject &b)
                                                       { return b.data.bullet.state == BulletState::inactive; }),
                                        gs.bullets[i].end());
                }
            }

            if (!gs.layers[LAYER_IDX_BODY].empty())
            {
                delectDeadBody(gs);
            }

            if (gs.player().data.player.currentHealth == 0)
            {
                playSound(res.lost);
            }
        }
        else
        {
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
                }
            }
        }

        for (auto &bullet : gs.bullets)
        {
            for (auto &b : bullet)
                drawObject(state, gs, b, deltaTime);
        }

        for (auto &layer : gs.layers)
        {
            for (GameObject &obj : layer)
            {
                drawObject(state, gs, obj, deltaTime);
            }
        }

        updateMapViewPort(state, gs, gs.player(), deltaTime);

        if (debug)
        {
            writeDebugText(state, gs, deltaTime);
        }
        drawUI(state, gs, res);
        SDL_RenderPresent(state._renderer);
        if (frameDelay > deltaTime)
        {
            SDL_Delay(frameDelay - deltaTime);
        }
    }

    std::cout << "\nhello world!\n";

    res.unload();
    state.~State();
    TTF_Quit();
    MIX_Quit();
    SDL_Quit();
    return 0;
}