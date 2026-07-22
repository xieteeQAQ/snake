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
#include "Animation.hpp"

extern bool debug;
extern bool collision_box;
extern std::random_device rd;
extern float playtime;

constexpr size_t GROUP_INDEX_BGM = 0;
constexpr size_t GROUP_INDEX_SPRING = 1;
constexpr size_t GROUP_INDEX_EAT = 2;
constexpr size_t GROUP_INDEX_PLAYERHURT = 3;
struct Resources
{
    const size_t ANIM_POTATO_GROW = 0;
    const size_t ANIM_POTATO_IDLE = 1;
    std::vector<Animation> potatoAnims;

    std::vector<SDL_Texture *> texs;
    SDL_Texture *tex_standby, *food, *background, *QAQ, *body, *potato_0, *potato_1, *potato_2, *potato_boom;

    std::vector<std::vector<MIX_Track*>> groups;
    std::vector<MIX_Track*> tracks;
    MIX_Track *Graze_The_Roof, *spring_1, *spring_2, *eat_1, *eat_2, *eat_3, *burp, *potato_boom_sound, *planting_sound,
    *plant_rise, *lost, *ah1, *ah2, *ah3, *wo, *heal_sound;

    SDL_Texture *loadTex(SDL_Renderer *renderer, const std::string &filename)
    {
        SDL_Texture *tex = IMG_LoadTexture(renderer, filename.c_str());
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        texs.push_back(tex);
        return tex;
    }

    MIX_Track *loadAudio(MIX_Mixer *mixer, std::vector<MIX_Track*> &vec, const std::string &filename)
    {
        MIX_Audio *audio = MIX_LoadAudio(mixer, filename.c_str(), false);
        MIX_Track *track = MIX_CreateTrack(mixer);
        MIX_SetTrackAudio(track, audio);
        vec.push_back(track);
        MIX_DestroyAudio(audio);
        return track;
    }

    void load(State &state, MIX_Mixer *mixer)
    {
        potatoAnims.resize(2);
        potatoAnims[ANIM_POTATO_GROW] = Animation(16, 0.59);
        potatoAnims[ANIM_POTATO_IDLE] = Animation(16, 0.59);

        tex_standby = loadTex(state._renderer, "image/player_normal.png");
        food = loadTex(state._renderer, "image/otto.png");
        background = loadTex(state._renderer, "image/Frontyard.webp");
        QAQ = loadTex(state._renderer, "image/QAQ.png");
        body = loadTex(state._renderer, "image/body_chicken.png");
        potato_0 = loadTex(state._renderer, "image/potato/potato_0.png");
        potato_1 = loadTex(state._renderer, "image/potato/potato_1.png");
        potato_2 = loadTex(state._renderer, "image/potato/potato_2.png");
        potato_boom = loadTex(state._renderer, "image/potato/potato_boom.png");
        
        std::vector<MIX_Track*> bgm_group;
        std::vector<MIX_Track*> spring_group;
        std::vector<MIX_Track*> eat_group;
        std::vector<MIX_Track*> playerHurt_group;
        Graze_The_Roof = loadAudio(mixer, bgm_group, "music/Graze_The_Roof.mp3");
        spring_1 = loadAudio(mixer, spring_group, "music/otto_spring_1.wav");
        spring_2 = loadAudio(mixer, spring_group, "music/otto_spring_2.wav");
        eat_1 = loadAudio(mixer, eat_group, "music/Eat1.ogg");
        eat_2 = loadAudio(mixer, eat_group, "music/Eat2.ogg");
        eat_3 = loadAudio(mixer, eat_group, "music/Eat3.ogg");
        burp = loadAudio(mixer, tracks, "music/Burp.ogg");
        potato_boom_sound = loadAudio(mixer, tracks, "music/potato_boom.mp3");
        planting_sound = loadAudio(mixer, tracks, "music/planting_sound.mp3");
        plant_rise = loadAudio(mixer, tracks, "music/plant_rise.mp3");
        lost = loadAudio(mixer, tracks, "music/lost.mp3");
        ah1 = loadAudio(mixer, playerHurt_group, "music/otto_hurt/ah1.wav");
        ah2 = loadAudio(mixer, playerHurt_group, "music/otto_hurt/ah2.wav");
        ah3 = loadAudio(mixer, playerHurt_group, "music/otto_hurt/ah3.wav");
        wo = loadAudio(mixer, playerHurt_group, "music/otto_hurt/wo.wav");
        heal_sound = loadAudio(mixer, tracks, "music/undertale_heal_sound.mp3");

        groups.push_back(bgm_group);
        groups.push_back(spring_group);
        groups.push_back(eat_group);
        groups.push_back(playerHurt_group);
    }

    void unload()
    {
        for (auto t : texs)
        {
            SDL_DestroyTexture(t);
        }
        for (auto g : groups)
        {
            for (auto t : g)
            {
                MIX_DestroyTrack(t);
            }
        }
        for (auto t : tracks)
        {
            MIX_DestroyTrack(t);
        }
    }
};

constexpr size_t LAYER_IDX_LEVEL = 0;
constexpr size_t LAYER_IDX_BODY = 1;
constexpr size_t LAYER_IDX_CHARACTERS = 2;
constexpr size_t LAYER_IDX_FOOD = 3;

constexpr size_t BULLET_IDX_POTATO = 0;
struct GameState
{
    std::array<std::vector<GameObject>, 4> layers;
    std::array<std::vector<GameObject>, 1> bullets;
    int playerIndex;
    int food_count;
    int potato_count;
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
        potato_count = 0;
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
void playBGM(MIX_Track *track, float volume = 0.3f);
void playSound(MIX_Track *track, float volume = 0.3f);
void playSound(std::vector<MIX_Track*> &group, int index, float volume = 0.3f);
void generatePotatoMine(State &state, GameState &gs, Resources &res, float deltaTime);
void drawUI(State &state, GameState &gs);
void drawPlayerHealth(State &state, GameState &gs);