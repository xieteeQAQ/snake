#pragma once

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "gaobject.hpp"
#include "Timer.hpp"
#include "State.hpp"
#include "Animation.hpp"
#include "BulletGenerater.hpp"

struct BulletGenerater;

extern bool debug;
extern bool collision_box;
extern std::random_device rd;
extern float playtime;
extern const float TILE_SIZE;

extern const float LEFTEDGE;
extern const float RIGHTEDGE;
extern const float UPPEREDGE;
extern const float LOWERLEFTEDGE;

constexpr size_t GROUP_INDEX_BGM = 0;
constexpr size_t GROUP_INDEX_SPRING = 1;
constexpr size_t GROUP_INDEX_EAT = 2;
constexpr size_t GROUP_INDEX_PLAYERHURT = 3;
struct Resources
{
    const size_t ANIM_POTATO_GROW = 0;
    const size_t ANIM_POTATO_IDLE = 1;
    const size_t ANIM_STICKYRICE_SPIN = 2;
    std::vector<Animation> bulletAnims;

    std::vector<SDL_Texture *> texs;
    SDL_Texture *tex_standby, *food, *background, *QAQ, *body, *potato_0, *potato_1, *potato_2, *potato_boom, *stickyRice,
        *warning, *nailong;

    std::vector<std::vector<MIX_Track *>> groups;
    std::vector<MIX_Track *> tracks;
    MIX_Track *Graze_The_Roof, *spring_1, *spring_2, *eat_1, *eat_2, *eat_3, *burp, *potato_boom_sound, *planting_sound,
        *plant_rise, *lost, *ah1, *ah2, *ah3, *wo, *heal_sound;

    std::vector<TTF_Font *> fonts;
    TTF_Font *hpFont;

    SDL_Texture *loadTex(SDL_Renderer *renderer, const std::string &filename)
    {
        SDL_Texture *tex = IMG_LoadTexture(renderer, filename.c_str());
        if (!tex)
        {
            SDL_Log("IMG: %s", SDL_GetError());
        }
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        texs.push_back(tex);
        return tex;
    }

    MIX_Track *loadAudio(MIX_Mixer *mixer, std::vector<MIX_Track *> &vec, const std::string &filename)
    {
        MIX_Audio *audio = MIX_LoadAudio(mixer, filename.c_str(), false);
        if (!audio)
        {
            SDL_Log("MIX: %s", SDL_GetError());
        }
        MIX_Track *track = MIX_CreateTrack(mixer);
        MIX_SetTrackAudio(track, audio);
        vec.push_back(track);
        MIX_DestroyAudio(audio);
        return track;
    }

    TTF_Font *loadFont(const std::string &filename, float ptsize)
    {
        TTF_Font *font = TTF_OpenFont(filename.c_str(), ptsize);
        if (!font)
        {
            SDL_Log("TTF: %s", SDL_GetError());
        }
        fonts.push_back(font);
        return font;
    }

    void load(State &state)
    {
        bulletAnims.resize(3);
        bulletAnims[ANIM_POTATO_GROW] = Animation(16, 0.59);
        bulletAnims[ANIM_POTATO_IDLE] = Animation(16, 0.59);
        bulletAnims[ANIM_STICKYRICE_SPIN] = Animation(2, 0.3);

        tex_standby = loadTex(state._renderer, "image/player_normal.png");
        food = loadTex(state._renderer, "image/otto.png");
        background = loadTex(state._renderer, "image/Frontyard.png");
        QAQ = loadTex(state._renderer, "image/QAQ.png");
        body = loadTex(state._renderer, "image/body_chicken.png");
        potato_0 = loadTex(state._renderer, "image/bullets/potato/potato_0.png");
        potato_1 = loadTex(state._renderer, "image/bullets/potato/potato_1.png");
        potato_2 = loadTex(state._renderer, "image/bullets/potato/potato_2.png");
        potato_boom = loadTex(state._renderer, "image/bullets/potato/potato_boom.png");
        stickyRice = loadTex(state._renderer, "image/bullets/stickyRice.png");
        warning = loadTex(state._renderer, "image/warning.png");
        nailong = loadTex(state._renderer, "image/nailong.png");

        std::vector<MIX_Track *> bgm_group;
        std::vector<MIX_Track *> spring_group;
        std::vector<MIX_Track *> eat_group;
        std::vector<MIX_Track *> playerHurt_group;
        Graze_The_Roof = loadAudio(state._mixer, bgm_group, "music/Graze_The_Roof.mp3");
        spring_1 = loadAudio(state._mixer, spring_group, "music/otto_spring_1.wav");
        spring_2 = loadAudio(state._mixer, spring_group, "music/otto_spring_2.wav");
        eat_1 = loadAudio(state._mixer, eat_group, "music/Eat1.ogg");
        eat_2 = loadAudio(state._mixer, eat_group, "music/Eat2.ogg");
        eat_3 = loadAudio(state._mixer, eat_group, "music/Eat3.ogg");
        burp = loadAudio(state._mixer, tracks, "music/Burp.ogg");
        potato_boom_sound = loadAudio(state._mixer, tracks, "music/potato_boom.mp3");
        planting_sound = loadAudio(state._mixer, tracks, "music/planting_sound.mp3");
        plant_rise = loadAudio(state._mixer, tracks, "music/plant_rise.mp3");
        lost = loadAudio(state._mixer, tracks, "music/lost.mp3");
        ah1 = loadAudio(state._mixer, playerHurt_group, "music/otto_hurt/ah1.wav");
        ah2 = loadAudio(state._mixer, playerHurt_group, "music/otto_hurt/ah2.wav");
        ah3 = loadAudio(state._mixer, playerHurt_group, "music/otto_hurt/ah3.wav");
        wo = loadAudio(state._mixer, playerHurt_group, "music/otto_hurt/wo.wav");
        heal_sound = loadAudio(state._mixer, tracks, "music/undertale_heal_sound.mp3");

        groups.push_back(bgm_group);
        groups.push_back(spring_group);
        groups.push_back(eat_group);
        groups.push_back(playerHurt_group);

        hpFont = loadFont("./font/Impact.ttf", 16);
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
        for (auto f : fonts)
        {
            TTF_CloseFont(f);
        }
    }
};

constexpr size_t LAYER_IDX_LEVEL = 0;
constexpr size_t LAYER_IDX_BODY = 1;
constexpr size_t LAYER_IDX_CHARACTERS = 2;
constexpr size_t LAYER_IDX_FOOD = 3;

constexpr size_t BULLET_IDX_POTATO = 0;
constexpr size_t BULLET_IDX_FRYING = 1;
constexpr size_t BULLET_IDX_TRACKING = 2;

constexpr size_t TIMER_PLAYER_HURT_SOUNDEFFTECT = 0;
struct GameState
{
    std::array<std::vector<GameObject>, 4> layers;
    std::array<std::vector<GameObject>, 3> bullets;
    std::vector<BulletGenerater> bulletGeneraters;
    std::vector<Timer> timers;
    int playerIndex;
    int food_count;
    int potato_count;
    int eat;
    int score;
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
        score = 0;
        n.type = ObjectType::nullobj;
        bodys_changed = false;

        timers.push_back(Timer(0.5f));
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

// check the point and correct it if it beyonds the map edge
void checkPointEdge(glm::vec2 &point);

// draw texture of the object, draw the debug information at same time
void drawObject(const State &state, GameState &gs, GameObject &obj, float deltaTime);

// create body when player meets growth requires
void createBody(const State &state, GameState &gs, Resources &res);

// if the object has a collision, give the corresponding response
void collisionResponse(const State &state, GameState &gs, Resources &res,
                       const SDL_FRect &recA, const SDL_FRect &recB, const SDL_FRect &recC,
                       GameObject &objA, GameObject &objB, float deltaTime);

// check the object collision and call the response function if it existed
void checkCollision(const State &state, GameState &gs, Resources &res,
                    GameObject &a, GameObject &b, float deltaTime);

// update the state of existed objects, check collisons amongs the objects, update the time of timers
void update(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);

// update function for player objects
void updatePlayer(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);

// update function for player's bodys objects
void updateBody(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);

// update function for bullets objects
void updateBullet(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);

// initialize the map and player
void createMap(const State &state, GameState &gs, const Resources &res);

// handle the key input
void handleKeyInput(const State &state, GameState &gs, GameObject &obj, Resources &res, float deltatime,
                    SDL_Scancode key, bool keydown);

// draw the map background
void drawBackground(State &state, GameState &gs, GameObject &obj, SDL_Texture *tex);

// generate food on the map
void generateFood(State &state, GameState &gs, Resources &res, float deltaTime);

// write debug infomation on the screen
void writeDebugText(State &state, GameState &gs, float deltaTime);

// play background music
void playBGM(MIX_Track *track, float volume = 0.3f);

// play sound effect
void playSound(MIX_Track *track, float volume = 0.3f);

// play sound effect in the resourses group(vector), if the index is -1, play randomly
void playSound(std::vector<MIX_Track *> &group, int index, float volume = 0.3f);

// generate potato mine on the map
void generatePotatoMine(State &state, GameState &gs, Resources &res, float deltaTime);

// draw game UI
void drawUI(State &state, GameState &gs, Resources &res);

// draw player's health informaion
void drawPlayerHealth(State &state, GameState &gs, Resources &res);

// draw score
void drawScore(State &state, GameState &gs, Resources &res);

// detect the object's position, and correct it if if beyonds the map edge
void edgeDetection(const State &state, GameState &gs, GameObject &obj);

// update the map viewport
void updateMapViewPort(State &state, GameState &gs, GameObject &obj, float deltatime);

// return true if the object beyonds the map edge
bool outOfRange(GameObject &obj);

// draw bullets warning
void drawWarning(const State &state, GameState &gs, Resources &res, glm::vec2 position);

// delect the body objects whose state is "dead"
void delectDeadBody(GameState &gs);