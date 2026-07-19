#pragma once

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <random>
#include <sstream>
#include <iomanip>
#include "gaobject.hpp"
#include "Timer.hpp"
#include "State.hpp"

extern bool debug;
extern bool collision_box;
extern std::random_device rd;

constexpr size_t GROUP_INDEX_BGM = 0;
constexpr size_t GROUP_INDEX_SPRING = 1;
struct Resources
{
    std::vector<SDL_Texture *> texs;
    SDL_Texture *tex_standby, *food, *background, *QAQ, *body;

    std::vector<std::vector<MIX_Track*>> tracks;
    MIX_Track *Graze_The_Roof, *spring_1, *spring_2;

    SDL_Texture *loadTex(SDL_Renderer *renderer, const std::string &filename)
    {
        SDL_Texture *tex = IMG_LoadTexture(renderer, filename.c_str());
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        texs.push_back(tex);
        return tex;
    }

    MIX_Track *loadAudio(MIX_Mixer *mixer, std::vector<MIX_Track*> &group, const std::string &filename)
    {
        MIX_Audio *audio = MIX_LoadAudio(mixer, filename.c_str(), false);
        MIX_Track *track = MIX_CreateTrack(mixer);
        MIX_SetTrackAudio(track, audio);
        group.push_back(track);
        return track;
    }

    void load(State &state, MIX_Mixer *mixer)
    {
        tex_standby = loadTex(state._renderer, "image/player_normal.png");
        food = loadTex(state._renderer, "image/otto.png");
        background = loadTex(state._renderer, "image/Frontyard.webp");
        QAQ = loadTex(state._renderer, "image/QAQ.png");
        body = loadTex(state._renderer, "image/body_chicken.png");

        std::vector<MIX_Track*> bgm_group;
        std::vector<MIX_Track*> spring_group;
        Graze_The_Roof = loadAudio(mixer, bgm_group, "music/Graze_The_Roof.mp3");
        spring_1 = loadAudio(mixer, spring_group, "music/otto_spring_1.wav");
        spring_2 = loadAudio(mixer, spring_group, "music/otto_spring_2.wav");

        tracks.push_back(bgm_group);
        tracks.push_back(spring_group);
    }

    void unload()
    {
        for (auto t : texs)
        {
            SDL_DestroyTexture(t);
        }
        for (auto g : tracks)
        {
            for (auto t : g)
            MIX_DestroyTrack(t);
        }
    }
};

constexpr size_t LAYER_IDX_LEVEL = 0;
constexpr size_t LAYER_IDX_BODY = 1;
constexpr size_t LAYER_IDX_CHARACTERS = 2;
constexpr size_t LAYER_IDX_FOOD = 3;
struct GameState
{
    std::array<std::vector<GameObject>, 4> layers;
    int playerIndex;
    int food_count;
    int eat;
    SDL_FRect mapViewport;
    GameObject n;
    bool bodys_changed;

    GameState(State &state)
    {
        playerIndex = -1;
        mapViewport = {
            .x = 0, .y = 0, .w = static_cast<float>(state.logW), .h = static_cast<float>(state.logH)};
        food_count = 0;
        eat = 0;
        n.type = ObjectType::nullobj;
        bodys_changed = false;
    }

    GameObject &player()
    {
        return layers[LAYER_IDX_CHARACTERS][playerIndex];
    }

    GameObject &lastBody()
    {
        if (layers[LAYER_IDX_BODY].empty())
        {
            return n;
        }
        return layers[LAYER_IDX_BODY][layers[LAYER_IDX_BODY].size() - 1];
    }

    GameObject &body(size_t idx)
    {
        if (idx >= layers[LAYER_IDX_BODY].size())
        {
            if (layers[LAYER_IDX_BODY].empty())
                return n;
            else
                return lastBody();
        }
        return layers[LAYER_IDX_BODY][idx];
    }

    void bodysSort()
    {
        int num = 0;
        for (auto &b : layers[LAYER_IDX_BODY])
        {
            b.data.body.number = num;
            ++num;
        }
        bodys_changed = false;
    }
};

void checkPointEdge(glm::vec2 &point);
void drawObject(const State &state, GameState &gs, GameObject &obj, float deltaTime);
void createBody(const State &state, GameState &gs, Resources &res);
void collisionResponse(const State &state, GameState &gs, Resources &res,
                       const SDL_FRect &recA, const SDL_FRect &recB, const SDL_FRect &recC,
                       GameObject &objA, GameObject &objB, float deltaTime);
void checkCollision(const State &state, GameState &gs, Resources &res,
                    GameObject &a, GameObject &b, float deltaTime);
void update(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);
void createMap(const State &state, GameState &gs, const Resources &res);
void handleKayInput(const State &state, GameState &gs, GameObject &obj, Resources &res, float deltatime,
                    SDL_Scancode key, bool keydown);
void drawBackground(State &state, GameState &gs, GameObject &obj, SDL_Texture *tex);
void generateFood(State &state, GameState &gs, Resources &res, float deltaTime);
void writeDebugText(State &state, GameState &gs, float deltaTime);
void playBGM(MIX_Track *track);
void playSound(MIX_Track *track);
void playSound(std::vector<MIX_Track*> &group, int index);