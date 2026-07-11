#pragma once

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <random>
#include "gaobject.hpp"
#include "Timer.hpp"
#include "State.hpp"

struct Resources
{
    std::vector<SDL_Texture*> texs;
    SDL_Texture *tex_standby, *food, *background, *QAQ;

    SDL_Texture *loadTex(SDL_Renderer *renderer, const std::string &filename)
    {
        SDL_Texture *tex = IMG_LoadTexture(renderer, filename.c_str());
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        texs.push_back(tex);
        return tex;
    }

    void load(State &state)
    {
        tex_standby = loadTex(state._renderer, "image/player_normal.png");
        food = loadTex(state._renderer, "image/otto.png");
        background = loadTex(state._renderer, "image/Frontyard.webp");
        QAQ = loadTex(state._renderer, "image/QAQ.png");
    }

    void unload()
    {
        for (auto t : texs)
        SDL_DestroyTexture(t);
    }
};

constexpr size_t LAYER_IDX_LEVEL = 0;
constexpr size_t LAYER_IDX_CHARACTERS = 1;
constexpr size_t LAYER_IDX_FOOD = 2;

struct GameState
{
    std::array<std::vector<GameObject>, 3> layers;
    int playerIndex;
    SDL_FRect mapViewport;

    GameState(State &state)
    {
        playerIndex = -1;
        mapViewport = {
            .x = 0, .y = 0,
            .w = static_cast<float>(state.logW), .h = static_cast<float>(state.logH)
        };
    }

    GameObject &player()
    {
        return layers[LAYER_IDX_CHARACTERS][playerIndex];
    }
};

constexpr float TILE_SIZE = 32;

void drawObject(const State &state, GameState &gs, GameObject &obj, float deltaTime)
{
    const float spriteSize = 128;
    SDL_FRect src{.x = 0, .y = 0, .w = spriteSize, .h = spriteSize};
    SDL_FRect dst{.x = obj.position.x - gs.mapViewport.x, .y = obj.position.y - gs.mapViewport.y, .w = TILE_SIZE, .h = TILE_SIZE};
    SDL_FPoint cen{.x = TILE_SIZE / 2, .y = TILE_SIZE / 2};

    SDL_FlipMode flipMode = obj.directionX == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state._renderer, obj.tex, &src, &dst, obj.angle, &cen, flipMode);
}


void collisionResponse(const State &state, GameState &gs, Resources &res, 
    const SDL_FRect &recA, const SDL_FRect &recB, const SDL_FRect &recC,
    GameObject &objA, GameObject &objB, float deltaTime)
{
    if (objA.type == ObjectType::player)
    {
        switch (objB.type)
        {
        case ObjectType::level:
        {
            if (recC.w < recC.h)
            {
                if (objA.velocity.x > 0)
                    objA.position.x -= recC.w;
                else if (objA.velocity.x < 0)
                    objA.position.x += recC.w;
                objA.velocity.x = 0;
            }
            else
            {
                if (objA.velocity.y > 0)
                    objA.position.y -= recC.h;
                else if (objA.velocity.y < 0)
                    objA.position.y += recC.h;
                objA.velocity.y = 0;
            }
            break;
        }
        
        default:
            break;
        }
    }
}

void checkCollision(const State &state, GameState &gs, Resources &res, 
    GameObject &a, GameObject &b, float deltaTime)
{
    SDL_FRect rectA{.x = a.position.x + a.collider.x, .y = a.position.y + a.collider.y, .w = a.collider.w, .h = a.collider.h};
    SDL_FRect rectB{.x = b.position.x + b.collider.x, .y = b.position.y + b.collider.y, .w = b.collider.w, .h = b.collider.h};
    SDL_FRect rectC{0};

    if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC))
    {
        collisionResponse(state, gs, res, rectA, rectB, rectC, a, b, deltaTime);
    }
}

void update(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
{
    if (obj.type == ObjectType::player)
    {   
        float currentDirectionX = 0;
        float currentDirectionY = 0;
        if (state.keys[SDL_SCANCODE_W])
            currentDirectionY -= 1;
        if (state.keys[SDL_SCANCODE_S])
            currentDirectionY += 1;
        if (state.keys[SDL_SCANCODE_A])
            currentDirectionX -= 1;
        if (state.keys[SDL_SCANCODE_D])
            currentDirectionX += 1;
        
        obj.directionX = currentDirectionX ? currentDirectionX : obj.directionX;
        obj.directionY = currentDirectionY ? currentDirectionY : obj.directionY;

        switch (obj.data.player.state)
        {
        case PlayerState::standby:
        {
            if (currentDirectionX || currentDirectionY)
            {
                obj.data.player.state = PlayerState::running;
            }
            else
            {
                if (!currentDirectionX)
                {
                    const float factorX = obj.velocity.x > 0 ? -2.5f : 2.5f;
                    float amountX = obj.acceleration.x * factorX * deltaTime;
                    obj.velocity.x = std::fabsf(obj.velocity.x) < std::fabsf(amountX) ? 0 : obj.velocity.x + amountX;
                }
                if (!currentDirectionY)
                {
                    const float factorY = obj.velocity.y > 0 ? -2.5f : 2.5f;
                    float amountY = obj.acceleration.y * factorY * deltaTime;
                    obj.velocity.y = std::fabsf(obj.velocity.y) < std::fabsf(amountY) ? 0 : obj.velocity.y + amountY;
                }
            }
            break;
        }
        case PlayerState::running:
        {
            if (currentDirectionX || currentDirectionY)
            {
                if (!currentDirectionX)
                {
                    const float factorX = obj.velocity.x > 0 ? -1.5f : 1.5f;
                    float amountX = obj.acceleration.x * factorX * deltaTime;
                    obj.velocity.x = std::fabsf(obj.velocity.x) < std::fabsf(amountX) ? 0 : obj.velocity.x + amountX;
                }
                if (!currentDirectionY)
                {
                    const float factorY = obj.velocity.y > 0 ? -1.5f : 1.5f;
                    float amountY = obj.acceleration.y * factorY * deltaTime;
                    obj.velocity.y = std::fabsf(obj.velocity.y) < std::fabsf(amountY) ? 0 : obj.velocity.y + amountY;
                }
            }
            if (!currentDirectionX && !currentDirectionY)
            {
                obj.data.player.state = PlayerState::standby;
            }
        }
        }

        obj.data.player.skills.updateSkills(deltaTime);

        std::string debug = std::to_string(obj.acceleration.x) +
                            "\n" + std::to_string(obj.velocity.x) + "\n" + std::to_string(obj.data.player.state) +
                            "\n" + std::to_string(obj.data.player.skills.skill_sprint.state) +
                            "\n" + std::to_string(obj.position.x) + "\n" + std::to_string(obj.position.y);
        SDL_SetRenderDrawColor(state._renderer, 0, 0, 0, 255);
        SDL_RenderDebugText(state._renderer, 5, 20, debug.c_str());

        obj.velocity.x += currentDirectionX * obj.acceleration.x * deltaTime;
        obj.velocity.x = std::fabsf(obj.velocity.x) > obj.maxSpeedX ? currentDirectionX * obj.maxSpeedX : obj.velocity.x;

        obj.velocity.y += currentDirectionY * obj.acceleration.y * deltaTime;
        obj.velocity.y = std::fabsf(obj.velocity.y) > obj.maxSpeedY ? currentDirectionY * obj.maxSpeedY : obj.velocity.y;

        obj.position += obj.velocity * deltaTime;
    }

    for (auto &layer : gs.layers)
    {
        for (GameObject &objB : layer)
        {
            checkCollision(state, gs, res, obj, objB, deltaTime);
        }
    }
}

constexpr int MAP_ROWS = 48;
constexpr int MAP_COLS = 60;
void createMap(const State &state, GameState &gs, const Resources &res)
{
    short map[MAP_ROWS][MAP_COLS] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0}
    };

    const auto createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type)
    {
        GameObject o;
        o.setType(type);
        o.position = glm::vec2(c * TILE_SIZE, r * TILE_SIZE);
        o.tex = tex;
        o.collider = {.x = 0, .y = 0, .w = TILE_SIZE, .h = TILE_SIZE};
        return o;
    };

    for (int r = 0; r < MAP_ROWS; ++r)
    {
        for (int c = 0; c < MAP_COLS; ++c)
        {
            switch (map[r][c])
            {
            case 2:
            {
                GameObject player = createObject(r, c, res.tex_standby, ObjectType::player);
                player.acceleration = glm::vec2(200, 200);
                player.maxSpeedX = 125;
                player.maxSpeedY = 125;
                player.collider = {.x = 8, .y = 4, .w = 16, .h = 23};
                gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
                gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
                break;
            }
            case 1:
            {
                GameObject QAQ = createObject(r, c, res.QAQ, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(QAQ);
                break;
            }
            default:
                break;
            }
        }
    }
    assert(gs.playerIndex != -1);
}

void handleKayInput(const State &state, GameState &gs, GameObject &obj, float deltatime,
    SDL_Scancode key, bool keydown)
{
    if (obj.type == ObjectType::player)
    {
        const float running_force = 200;
        switch (key)
        {
        case SDL_SCANCODE_F:
        {
            if (obj.data.player.state == PlayerState::running)
                obj.data.player.skills.sprint();
            break;
        }
        default:
            break;
        }
    }
}

void drawBackground(State &state, GameState &gs, GameObject &obj, SDL_Texture *tex)
{
    if (obj.type == ObjectType::player)
    {
        const float backgroundW = 1000;
        const float backgroundH = 429;
        const float size = 2.5;
        SDL_FRect src = {.x = 0, .y = 0, .w = backgroundW , .h = backgroundH};
        SDL_FRect dst = {.x = - gs.mapViewport.x, .y = - gs.mapViewport.y, .w = backgroundW * size, .h = backgroundH * size};
        SDL_RenderTextureRotated(state._renderer, tex, &src, &dst, 0, nullptr, SDL_FLIP_NONE);
    }
}

void generateFood(State &state, GameState &gs, Resources &res, float deltaTime)
{
    static Timer interval(3);
    if (interval.isTimeout())
    {
        interval.reset();
    }
    else
    {
        interval.step(deltaTime);
        return;
    }

    std::random_device rd;
    std::mt19937 generater(rd());
    std::normal_distribution<float> distributionX(1088.0f, 500.0f);
    std::normal_distribution<float> distributionY(576.5f, 500.0f);

    float x = 0, y = 0;
    while (x <= 440 || x >= 1736)
    {
        x = distributionX(generater);
    }
    while (y <= 156 || y >= 998)
    {
        y = distributionY(generater);
    }

    GameObject food;
    food.type = ObjectType::food;
    food.tex = res.food;
    food.position = glm::vec2(x, y);

    gs.layers[LAYER_IDX_FOOD].push_back(food);
}