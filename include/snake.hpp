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
    SDL_Texture *_tex;

    SDL_Texture *loadTex(SDL_Renderer *renderer, const std::string &filename)
    {
        _tex = IMG_LoadTexture(renderer, filename.c_str());
        SDL_SetTextureScaleMode(_tex, SDL_SCALEMODE_NEAREST);
        return _tex;
    }

    void unload()
    {
        SDL_DestroyTexture(_tex);
    }
};

constexpr size_t LAYER_IDX_LEVEL = 0;
constexpr size_t LAYER_IDX_CHARACTERS = 1;

struct GameState
{
    std::array<std::vector<GameObject>, 2> layers;
    int playerIndex;

    GameState()
    {
        playerIndex = 0;
    }
};

void drawObject(const State &state, GameState &gs, GameObject &obj, float deltaTime)
{
    const float spriteSize = 128;
    SDL_FRect src{.x = 0, .y = 0, .w = spriteSize, .h = spriteSize};
    SDL_FRect dst{.x = obj.position.x, .y = obj.position.y, .w = 64, .h = 64};
    SDL_FPoint cen{.x = 32, .y = 32};

    SDL_FlipMode flipMode = obj.directionX < 0 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state._renderer, obj.tex, &src, &dst, obj.angle, &cen, flipMode);
}

void update(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
{
    if (obj.type == ObjectType::player)
    {
        float mouseX = 0, mouseY = 0;
        static float destinationX = state.logX / 2.0f;
        static float destinationY = state.logY / 2.0f;

        Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
        if ((mouseButtons & SDL_BUTTON_LMASK) != 0)
        {
            SDL_RenderCoordinatesFromWindow(state._renderer, mouseX, mouseY, &destinationX, &destinationY);
        }

        const float spriteCenterX = obj.position.x + 32.0f;
        const float spriteCenterY = obj.position.y + 32.0f;
        const float deltaX = destinationX - spriteCenterX;
        const float deltaY = destinationY - spriteCenterY;

        const float directionDeadZone = 2.0f;
        if (deltaX > directionDeadZone)
            obj.directionX = 1.0f;
        else if (deltaX < -directionDeadZone)
            obj.directionX = -1.0f;
        else
            obj.directionX = 0.0f;

        if (deltaY > directionDeadZone)
            obj.directionY = 1.0f;
        else if (deltaY < -directionDeadZone)
            obj.directionY = -1.0f;
        else
            obj.directionY = 0.0f;

        static Timer rushCD(2.25);
        static Timer rushDuration(0.25);
        switch (obj.data.player.state)
        {
        case PlayerState::standby:
        {
            if (obj.velocity.x != 0 || obj.velocity.y != 0)
            {
                obj.data.player.state = PlayerState::running;
            }
            break;
        }
        case PlayerState::running:
        {
            if (obj.velocity.x == 0 && obj.velocity.y == 0)
            {
                obj.data.player.state = PlayerState::standby;
                break;
            }
            if (state.keys[SDL_SCANCODE_J])
            {
                obj.data.player.skills.sprint();
            }
            break;
        }
        }

        obj.data.player.skills.updateSkills(deltaTime);

        std::string debug = std::to_string(obj.acceleration.x) +
                            "\n" + std::to_string(obj.velocity.x) + "\n" + std::to_string(obj.data.player.state) + "\n" + std::to_string(obj.data.player.skills.skill_sprint.state);
        SDL_SetRenderDrawColor(state._renderer, 0, 0, 0, 255);
        SDL_RenderDebugText(state._renderer, 0, 0, debug.c_str());

        const float distance = std::sqrtf(deltaX * deltaX + deltaY * deltaY);
        const float arrivalThreshold = 10.0f;
        if (std::fabsf(deltaX) > arrivalThreshold)
        {
            obj.velocity.x += obj.directionX * obj.acceleration.x * deltaTime;
            obj.velocity.x = std::fabsf(obj.velocity.x) > obj.maxSpeedX ? obj.directionX * obj.maxSpeedX : obj.velocity.x;
        }
        else
        {
            obj.velocity.x = 0;
        }
        if (std::fabsf(deltaY) > arrivalThreshold)
        {
            obj.velocity.y += obj.directionY * obj.acceleration.y * deltaTime;
            obj.velocity.y = std::fabsf(obj.velocity.y) > obj.maxSpeedY ? obj.directionY * obj.maxSpeedY : obj.velocity.y;
        }
        else
        {
            obj.velocity.y = 0;
        }

        obj.position += obj.velocity * deltaTime;
    }
}