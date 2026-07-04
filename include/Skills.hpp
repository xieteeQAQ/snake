#pragma once

#include <vector>
#include <functional>
#include <utility>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include "Timer.hpp"

struct GameObject;
struct PlayerSkills;

enum SkillState
{
    IS_COOLING_DOWN,
    IS_AVAILABLE,
    IS_IN_EFFECT,
    IS_EXPIRED
};

struct Skills
{
    Skills(const float &_cool_down_time, const float &_duration, std::function<void(PlayerSkills &)> _apply_function, const SkillState &_state);
    void update(const float &deltatime);

    SkillState state;
    std::function<void(PlayerSkills &)> apply_function;
    Timer cool_down_time{0};
    Timer duration{0};
};

struct PlayerSkills
{
    PlayerSkills();
    PlayerSkills(const PlayerSkills &other);
    PlayerSkills(PlayerSkills &&other) noexcept;
    PlayerSkills &operator=(const PlayerSkills &other);
    PlayerSkills &operator=(PlayerSkills &&other) noexcept;
    void bind(GameObject &owner);
    void updateSkills(const float &deltatime);
    void sprint();

    std::vector<Skills *> skills;
    glm::vec2 ori_velocity, ori_acceleration;
    float ori_maxSpeedX;
    float ori_maxSpeedY;
    float ori_directionX;
    float ori_directionY;
    float ori_angle;
    SDL_Texture *ori_tex = nullptr;
    GameObject *owner = nullptr;

    Skills skill_sprint{2.0f, 0.25f, &PlayerSkills::sprint, SkillState::IS_AVAILABLE};
};
