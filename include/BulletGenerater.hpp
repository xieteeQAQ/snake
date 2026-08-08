#pragma once

#include "snake.hpp"

extern std::random_device rd;
extern const float TILE_SIZE;

extern const float LEFTEDGE;
extern const float RIGHTEDGE;
extern const float UPPEREDGE;
extern const float LOWERLEFTEDGE;

void updateBulletGenerater(const State &state, GameState &gs, Resources &res, float deltaTime);

struct BulletGenerater
{
    enum Type
    {
        circleStickyRice,
        nailong
    };

    enum BgState
    {
        waiting,
        delaying,
        warning,
        ready
    };

    BulletGenerater() = delete;
    BulletGenerater(const float &delayTime, const float &warningTime, SDL_Texture *warningTex, Type type) : delayTime(delayTime), warningTime(warningTime), warningTex(warningTex), type(type) {
        std::mt19937 generater(rd());
        std::uniform_int_distribution<int> distX(LEFTEDGE, RIGHTEDGE);
        std::uniform_int_distribution<int> distY(UPPEREDGE, LOWERLEFTEDGE);
        position.x = static_cast<float>(distX(generater));
        position.y = static_cast<float>(distY(generater));
        alive = true;
        state = BgState::waiting;
    };
    BulletGenerater(const glm::vec2 &position, const float &delayTime, const float &warningTime, SDL_Texture *warningTex, Type type) : position(position), delayTime(delayTime), warningTime(warningTime), warningTex(warningTex), type(type) {
        alive = true;
        state = BgState::waiting;
    };
    bool WaringIsTimeOut() const { return warningTime.isTimeout(); };
    bool delayIsTimeOut() const { return delayTime.isTimeout(); };
    void stepDelayTime(float deltaTime) { delayTime.step(deltaTime); };
    void stepWarningTime(float deltaTime) { warningTime.step(deltaTime); };
    void resetDelayTime() { delayTime.reset(); };
    void resetWarningTime() { warningTime.reset(); };
    bool isAlive() const { return alive; };
    void setPosition(const glm::vec2 &_position) { position = _position; };
    void setDelayTime(const float &_delayTime) { delayTime = _delayTime; };
    void setWarningTime(const float &_warningTime) { warningTime = _warningTime; };
    void setWarningTex(SDL_Texture *_warningTex) { warningTex = _warningTex; };
    void putWarning(const State &state, GameState &gs);
    void createCircleBullet(const State &state, GameState &gs, Resources &res, SDL_Texture *tex, const glm::vec2 &position,glm::vec2 velocity, SDL_FRect collider, int attack, int amount);
    void createNaiLong(const State &state, GameState &gs, Resources &res, const glm::vec2 &position);
    void randomPosition();

    Type type;
    BgState state;
    glm::vec2 position;
    Timer warningTime;
    Timer delayTime;
    SDL_Texture *warningTex;
    bool alive;
};