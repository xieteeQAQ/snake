#pragma once

#include <new>
#include <utility>
#include <glm/glm.hpp>
#include <deque>
#include <SDL3_image/SDL_image.h>
#include "Skills.hpp"
#include "Timer.hpp"

enum PlayerState
{
    running,
    standby
};

enum BodyState
{
    activitive
};

struct BodyData
{
    BodyState state;
    Ruler ruler{5};
    std::deque<glm::vec2> points{};
    unsigned int number = 0;

    BodyData() : state(BodyState::activitive)
    {
    }
};

struct PlayerData
{
    PlayerState state;
    PlayerSkills skills;
    Ruler ruler{5};
    std::deque<glm::vec2> points{};
    Counter grow_counter{3};

    PlayerData() : state(PlayerState::standby)
    {
    }
};

struct LevelData
{
};

struct FoodData
{
    int number;

    FoodData() = default;
    FoodData(int num) : number(num)
    {
    }
};


enum ObjectType
{
    nullobj,
    player,
    body,
    food,
    level
};

struct ObjectData
{
    union
    {
        PlayerData player;
        LevelData level;
        FoodData food;
        BodyData body;
    };

    ObjectType kind;

    ObjectData() : kind(ObjectType::level), level()
    {
    }

    explicit ObjectData(ObjectType objectType) : kind(objectType)
    {
        switch (objectType)
        {
        case ObjectType::player:
            new (&player) PlayerData();
            break;
        case ObjectType::food:
            new (&food) FoodData();
            break;
        case ObjectType::body:
            new (&body) BodyData();
        case ObjectType::level:
        default:
            new (&level) LevelData();
            break;
        }
    }

    ObjectData(const ObjectData &other) : kind(other.kind)
    {
        switch (other.kind)
        {
        case ObjectType::player:
            new (&player) PlayerData(other.player);
            break;
        case ObjectType::food:
            new (&food) FoodData(other.food);
            break;
        case ObjectType::body:
            new (&body) BodyData(other.body);
            break;
        case ObjectType::level:
        default:
            new (&level) LevelData(other.level);
            break;
        }
    }

    ObjectData(ObjectData &&other) noexcept : kind(other.kind)
    {
        switch (other.kind)
        {
        case ObjectType::player:
            new (&player) PlayerData(std::move(other.player));
            break;
        case ObjectType::food:
            new (&food) FoodData(std::move(other.food));
            break;
        case ObjectType::body:
            new (&body) BodyData(std::move(other.body));
            break;
        case ObjectType::level:
        default:
            new (&level) LevelData(std::move(other.level));
            break;
        }
    }

    ObjectData &operator=(const ObjectData &other)
    {
        if (this != &other)
        {
            destroy();
            kind = other.kind;
            switch (other.kind)
            {
            case ObjectType::player:
                new (&player) PlayerData(other.player);
                break;
            case ObjectType::food:
                new (&food) FoodData(other.food);
                break;
            case ObjectType::body:
                new (&body) BodyData(other.body);
                break;
            case ObjectType::level:
            default:
                new (&level) LevelData(other.level);
                break;
            }
        }
        return *this;
    }

    ObjectData &operator=(ObjectData &&other) noexcept
    {
        if (this != &other)
        {
            destroy();
            kind = other.kind;
            switch (other.kind)
            {
            case ObjectType::player:
                new (&player) PlayerData(std::move(other.player));
                break;
            case ObjectType::food:
                new (&food) FoodData(std::move(other.food));
                break;
            case ObjectType::body:
                new (&body) BodyData(std::move(other.body));
                break;
            case ObjectType::level:
            default:
                new (&level) LevelData(std::move(other.level));
                break;
            }
        }
        return *this;
    }

    ~ObjectData()
    {
        destroy();
    }

private:
    void destroy()
    {
        switch (kind)
        {
        case ObjectType::player:
            player.~PlayerData();
            break;
        case ObjectType::food:
            food.~FoodData();
            break;
        case ObjectType::body:
            body.~BodyData();
            break;
        case ObjectType::level:
        default:
            level.~LevelData();
            break;
        }
    }
};

struct GameObject
{
    ObjectType type;
    ObjectData data;
    glm::vec2 position, velocity, acceleration;
    float maxSpeedX;
    float maxSpeedY;
    float directionX;
    float directionY;
    float angle;
    SDL_Texture *tex;
    SDL_FRect collider;

    GameObject()
        : type(ObjectType::level),
          data(ObjectType::level),
          position(0.0f, 0.0f),
          velocity(0.0f, 0.0f),
          acceleration(0.0f, 0.0f),
          maxSpeedX(0.0f),
          maxSpeedY(0.0f),
          directionX(1.0f),
          directionY(1.0f),
          angle(0.0f),
          tex(nullptr),
          collider{0}
    {
        bindSkills();
    }

    GameObject(const GameObject &other)
        : type(other.type),
          data(other.data),
          position(other.position),
          velocity(other.velocity),
          acceleration(other.acceleration),
          maxSpeedX(other.maxSpeedX),
          maxSpeedY(other.maxSpeedY),
          directionX(other.directionX),
          directionY(other.directionY),
          angle(other.angle),
          tex(other.tex),
          collider{other.collider}
    {
        bindSkills();
    }

    GameObject(GameObject &&other) noexcept
        : type(other.type),
          data(std::move(other.data)),
          position(other.position),
          velocity(other.velocity),
          acceleration(other.acceleration),
          maxSpeedX(other.maxSpeedX),
          maxSpeedY(other.maxSpeedY),
          directionX(other.directionX),
          directionY(other.directionY),
          angle(other.angle),
          tex(other.tex),
          collider{other.collider}
    {
        bindSkills();
    }

    GameObject &operator=(const GameObject &other)
    {
        if (this != &other)
        {
            type = other.type;
            data = other.data;
            position = other.position;
            velocity = other.velocity;
            acceleration = other.acceleration;
            maxSpeedX = other.maxSpeedX;
            maxSpeedY = other.maxSpeedY;
            directionX = other.directionX;
            directionY = other.directionY;
            angle = other.angle;
            tex = other.tex;
            collider = other.collider;
            bindSkills();
        }
        return *this;
    }

    GameObject &operator=(GameObject &&other) noexcept
    {
        if (this != &other)
        {
            type = other.type;
            data = std::move(other.data);
            position = other.position;
            velocity = other.velocity;
            acceleration = other.acceleration;
            maxSpeedX = other.maxSpeedX;
            maxSpeedY = other.maxSpeedY;
            directionX = other.directionX;
            directionY = other.directionY;
            angle = other.angle;
            tex = other.tex;
            collider = other.collider;
            bindSkills();
        }
        return *this;
    }

    void setType(ObjectType objectType)
    {
        if (type == objectType)
        {
            return;
        }

        type = objectType;
        switch (objectType)
        {
        case ObjectType::player:
            data = ObjectData(ObjectType::player);
            break;
        case ObjectType::food:
            data = ObjectData(ObjectType::food);
            break;
        case ObjectType::body:
            data = ObjectData(ObjectType::body);
            break;
        case ObjectType::level:
        default:
            data = ObjectData(ObjectType::level);
            break;
        }
        bindSkills();
    }

    ~GameObject() = default;

private:
    void bindSkills()
    {
        if (type == ObjectType::player)
        {
            data.player.skills.bind(*this);
        }
    }
};
