#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <thread>
#include <chrono>
#include "State.hpp"
#include "Food.hpp"
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
        std::cerr << "SDL: init failed\n";
        return 1;
    }

    if (!MIX_Init())
    {
        std::cerr << "SDL_mixer: init failed\n";
        return 1;
    }

    if (!TTF_Init())
    {
        SDL_Log("TTF init failed: %s", SDL_GetError());
    }   

    MIX_Mixer *mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!mixer)
    {
        std::cerr << "SDL_mixer: create mixer failed\n";
        return 1;
    }

    State state;
    state.init();
    SDL_SetRenderVSync(state._renderer,1);

    Resources res;
    res.load(state, mixer);

    GameState gs(state);
    createMap(state, gs, res);
    uint64_t prevTime = SDL_GetTicks();

    bool running = true;
    playBGM(res.Graze_The_Roof);
    while (running)
    {
        uint64_t nowTime = SDL_GetTicks();
        float deltaTime = (nowTime - prevTime) / 1000.0f;
        prevTime = nowTime;
        playtime += deltaTime;
        
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
                    handleKayInput(state, gs, gs.player(), res, deltaTime, event.key.scancode, true);
                    break;
                }
                case SDL_EVENT_KEY_UP:
                {
                    handleKayInput(state, gs, gs.player(), res, deltaTime, event.key.scancode, false);
                    break;
                }
                }
            }

            SDL_SetRenderDrawColor(state._renderer, 30, 30, 30, 255);
            SDL_RenderClear(state._renderer);
            drawBackground(state, gs, gs.player(), res.background);

            generateFood(state, gs, res, deltaTime);
            generatePotatoMine(state, gs, res, deltaTime);

            static Timer circleBullet_timer(3);
            static Timer warning_timer(1);
            static float X = 0;
            static float Y = 0;
            circleBullet_timer.step(deltaTime);
            static bool position = false;
            if (circleBullet_timer.isTimeout())
            {
                if (!position)
                {
                    std::mt19937 generater(rd());
                    std::uniform_int_distribution<int> distX(LEFTEDGE, RIGHTEDGE);
                    std::uniform_int_distribution<int> distY(UPPEREDGE, LOWERLEFTEDGE);
                    X = static_cast<float>(distX(generater));
                    Y = static_cast<float>(distY(generater));
                    position = true;
                }
                warning_timer.step(deltaTime);
                if (warning_timer.isTimeout())
                {
                    glm::vec2 v = {5.0f, 5.0f};
                    SDL_FRect c = {.x = 13, .y = 10, .w = 6, .h = 12};
                    createCircleBullet(state, gs, res, res.bullet_particle, X, Y, v, c, 10, 10, deltaTime);
                    warning_timer.reset();
                    circleBullet_timer.reset();
                    position = false;
                }
                else
                {
                    drawWarning(state, gs, res, glm::vec2{X, Y});
                }
            }
            
            for (auto &layer : gs.layers)
            {
                for (GameObject &obj : layer)
                {
                    update(state, gs, res, obj, deltaTime);
                    if (obj.currentAnimation != -1)
                    {
                        obj.animation[obj.currentAnimation].step(deltaTime);
                    }
                }
            }
            for (auto &bullet : gs.bullets)
            {
                for (auto &b : bullet)
                {
                    update(state, gs, res, b, deltaTime);
                    if (b.currentAnimation != -1)
                    {
                        b.animation[b.currentAnimation].step(deltaTime);
                    }
                }
                
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
        drawUI(state, gs);
        SDL_RenderPresent(state._renderer);
        if(frameDelay > deltaTime)
        {
            SDL_Delay(frameDelay - deltaTime);
        }
    }

    std::cout << "\nhello world!\n";

    res.unload();
    state.~State();
    MIX_DestroyMixer(mixer);
    SDL_Quit();
    return 0;
}