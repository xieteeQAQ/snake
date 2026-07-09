#pragma once

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "gaobject.hpp"
#include "Timer.hpp"
#include "State.hpp"

struct Resources
{
    std::vector<SDL_Texture*> texs;
    SDL_Texture *tex_standby, *food, *wall_1, *wall_2, *wall_3, *wall_4, *wall_5, *wall_6,
    *wall_7, *wall_8;

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
        wall_1 = loadTex(state._renderer, "image/wall_1.png");
        wall_2 = loadTex(state._renderer, "image/wall_2.png");
        wall_3 = loadTex(state._renderer, "image/wall_3.png");
        wall_4 = loadTex(state._renderer, "image/wall_4.png");
        wall_5 = loadTex(state._renderer, "image/wall_5.png");
        wall_6 = loadTex(state._renderer, "image/wall_6.png");
        wall_7 = loadTex(state._renderer, "image/wall_7.png");
        wall_8 = loadTex(state._renderer, "image/wall_8.png");
    }

    void unload()
    {
        for (auto t : texs)
        SDL_DestroyTexture(t);
    }
};

constexpr size_t LAYER_IDX_LEVEL = 0;
constexpr size_t LAYER_IDX_CHARACTERS = 1;

struct GameState
{
    std::array<std::vector<GameObject>, 2> layers;
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
                            "\n" + std::to_string(obj.velocity.x) + "\n" + std::to_string(obj.data.player.state) + "\n" + std::to_string(obj.data.player.skills.skill_sprint.state);
        SDL_SetRenderDrawColor(state._renderer, 0, 0, 0, 255);
        SDL_RenderDebugText(state._renderer, 0, 0, debug.c_str());

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

constexpr int MAP_ROWS = 10;
constexpr int MAP_COLS = 100;
void createMap(const State &state, GameState &gs, const Resources &res)
{
    short map[MAP_ROWS][MAP_COLS] = {
        {8, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
        {7, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
    };

    const auto createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type)
    {
        GameObject o;
        o.setType(type);
        o.position = glm::vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE);
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
            case 9:
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
                GameObject wall_1 = createObject(r, c, res.wall_1, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_1);
                break;
            }
            case 2:
            {
                GameObject wall_2 = createObject(r, c, res.wall_2, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_2);
                break;
            }
            case 3:
            {
                GameObject wall_3 = createObject(r, c, res.wall_3, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_3);
                break;
            }
            case 4:
            {
                GameObject wall_4 = createObject(r, c, res.wall_4, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_4);
                break;
            }
            case 5:
            {
                GameObject wall_5 = createObject(r, c, res.wall_5, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_5);
                break;
            }
            case 6:
            {
                GameObject wall_6 = createObject(r, c, res.wall_6, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_6);
                break;
            }
            case 7:
            {
                GameObject wall_6 = createObject(r, c, res.wall_7, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_6);
                break;
            }
            case 8:
            {
                GameObject wall_6 = createObject(r, c, res.wall_8, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(wall_6);
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