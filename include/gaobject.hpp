#pragma once

#include <new>
#include <utility>
#include <glm/glm.hpp>
#include <deque>
#include <SDL3_image/SDL_image.h>
#include "Skills.hpp"
#include "Timer.hpp"
#include "Animation.hpp"
struct Resources;
struct GameState;

enum PlayerState
{
    running,
    standby
};

enum BodyState
{
    activitive
};

enum BulletState
{
    inactive, moving, colliding, idle
};

enum BulletType
{
    potatoMine, none
};

struct BodyData
{
    BodyState state;
    Ruler ruler{32};
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
    Ruler ruler{32};
    std::deque<glm::vec2> points{};
    Counter grow_counter{3};
    int totalHealth;
    int baseHealth;
    int extraHealth;
    int currentHealth;

    PlayerData() : state(PlayerState::standby)
    {
    }

    void hurt(int amount, Resources &res);

    void treat(int amount, Resources &res);

    void increaseExtraHealth(int amount);

    void increaseBaseHealth(int amount);

    void consumeBody(GameState &gs, Resources &res, int amount = 3);
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

struct bulletData
{
    BulletState state;
    BulletType type;
    Timer timer;
    int number;
    int attack;

    bulletData() : state(BulletState::moving), type(BulletType::none) ,timer(0), number(0), attack(0) {};
    bulletData(float length) : state(BulletState::moving), type(BulletType::none),timer(length), number(0), attack(0) {};
};


enum ObjectType
{
    nullobj,
    player,
    body,
    food,
    level,
    bullet
};

struct ObjectData
{
    union
    {
        PlayerData player;
        LevelData level;
        FoodData food;
        BodyData body;
        bulletData bullet;
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
            break;
        case ObjectType::level:
            break;
        case ObjectType::bullet:
            new (&bullet) bulletData();
            break;
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
            break;
        case ObjectType::bullet:
            new (&bullet) bulletData(other.bullet);
            break;
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
            break;
        case ObjectType::bullet:
            new (&bullet) bulletData(std::move(other.bullet));
            break;
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
                break;
            case ObjectType::bullet:
                new (&bullet) bulletData(other.bullet);
                break;
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
                break;
            case ObjectType::bullet:
                new (&bullet) bulletData(std::move(other.bullet));
                break;
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
            break;
        case ObjectType::bullet:
            bullet.~bulletData();
            break;
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
    std::vector<Animation> animation;
    int currentAnimation;
    SDL_Texture *tex;
    SDL_FRect collider;
    bool collideable;

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
          currentAnimation(-1),
          collider{0},
          collideable{true}
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
          animation(other.animation),
          currentAnimation(other.currentAnimation),
          collider{other.collider},
          collideable{other.collideable}
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
          animation(other.animation),
          currentAnimation(other.currentAnimation),
          collider{other.collider},
          collideable{other.collideable}
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
            animation = other.animation;
            currentAnimation = other.currentAnimation;
            collider = other.collider;
            collideable = other.collideable;
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
            animation = other.animation;
            currentAnimation = other.currentAnimation;
            collider = other.collider;
            collideable = other.collideable;
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
            break;
        case ObjectType::bullet:
            data = ObjectData(ObjectType::bullet);
            break;
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
