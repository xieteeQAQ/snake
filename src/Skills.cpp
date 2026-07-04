#include "Skills.hpp"
#include "gaobject.hpp"

PlayerSkills::PlayerSkills()
    : ori_velocity(glm::vec2(0, 0)), ori_acceleration(glm::vec2(0, 0)), ori_maxSpeedX(0), ori_maxSpeedY(0),
      ori_directionX(1), ori_directionY(1), ori_angle(0), ori_tex(nullptr)
{
    skills.emplace_back(&skill_sprint);
}

PlayerSkills::PlayerSkills(const PlayerSkills &other)
    : ori_velocity(other.ori_velocity),
      ori_acceleration(other.ori_acceleration),
      ori_maxSpeedX(other.ori_maxSpeedX),
      ori_maxSpeedY(other.ori_maxSpeedY),
      ori_directionX(other.ori_directionX),
      ori_directionY(other.ori_directionY),
      ori_angle(other.ori_angle),
      ori_tex(other.ori_tex),
      owner(other.owner),
      skill_sprint(other.skill_sprint)
{
    skills.emplace_back(&skill_sprint);
}

PlayerSkills::PlayerSkills(PlayerSkills &&other) noexcept
    : ori_velocity(std::move(other.ori_velocity)),
      ori_acceleration(std::move(other.ori_acceleration)),
      ori_maxSpeedX(other.ori_maxSpeedX),
      ori_maxSpeedY(other.ori_maxSpeedY),
      ori_directionX(other.ori_directionX),
      ori_directionY(other.ori_directionY),
      ori_angle(other.ori_angle),
      ori_tex(other.ori_tex),
      owner(other.owner),
      skill_sprint(std::move(other.skill_sprint))
{
    skills.emplace_back(&skill_sprint);
}

PlayerSkills &PlayerSkills::operator=(const PlayerSkills &other)
{
    if (this != &other)
    {
        ori_velocity = other.ori_velocity;
        ori_acceleration = other.ori_acceleration;
        ori_maxSpeedX = other.ori_maxSpeedX;
        ori_maxSpeedY = other.ori_maxSpeedY;
        ori_directionX = other.ori_directionX;
        ori_directionY = other.ori_directionY;
        ori_angle = other.ori_angle;
        ori_tex = other.ori_tex;
        owner = other.owner;
        skill_sprint = other.skill_sprint;
        skills.clear();
        skills.emplace_back(&skill_sprint);
    }
    return *this;
}

PlayerSkills &PlayerSkills::operator=(PlayerSkills &&other) noexcept
{
    if (this != &other)
    {
        ori_velocity = std::move(other.ori_velocity);
        ori_acceleration = std::move(other.ori_acceleration);
        ori_maxSpeedX = other.ori_maxSpeedX;
        ori_maxSpeedY = other.ori_maxSpeedY;
        ori_directionX = other.ori_directionX;
        ori_directionY = other.ori_directionY;
        ori_angle = other.ori_angle;
        ori_tex = other.ori_tex;
        owner = other.owner;
        skill_sprint = std::move(other.skill_sprint);
        skills.clear();
        skills.emplace_back(&skill_sprint);
    }
    return *this;
}

void PlayerSkills::bind(GameObject &owner)
{
    this->owner = &owner;
}

void PlayerSkills::sprint()
{
    if (!owner)
    {
        return;
    }

    if (skill_sprint.state == SkillState::IS_AVAILABLE)
    {
        skill_sprint.state = SkillState::IS_IN_EFFECT;
        owner->acceleration += glm::vec2(1800.0f, 1800.0f);
        owner->maxSpeedX += 650.0f;
        owner->maxSpeedY += 650.0f;
    }
    else if (skill_sprint.state == SkillState::IS_EXPIRED)
    {
        skill_sprint.state = SkillState::IS_COOLING_DOWN;
        owner->acceleration -= glm::vec2(1800.0f, 1800.0f);
        owner->maxSpeedX -= 650.0f;
        owner->maxSpeedY -= 650.0f;
    }
}

void PlayerSkills::updateSkills(const float &deltatime)
{
    if (!owner)
    {
        return;
    }

    for (auto *skill : skills)
    {
        skill->update(deltatime);
        if (skill->state == SkillState::IS_EXPIRED)
        {
            skill->apply_function(*this);
            skill->state = SkillState::IS_COOLING_DOWN;
        }
    }
}

Skills::Skills(const float &_cool_down_time, const float &_duration, std::function<void(PlayerSkills &)> _apply_function, const SkillState &_state)
{
    cool_down_time.setLength(_cool_down_time);
    duration.setLength(_duration);
    apply_function = _apply_function;
    state = _state;
}

void Skills::update(const float &deltatime)
{
    switch (state)
    {
    case SkillState::IS_IN_EFFECT:
    {
        duration.step(deltatime);
        if (duration.isTimeout())
        {
            state = SkillState::IS_EXPIRED;
            duration.reset();
        }
        break;
    }
    case SkillState::IS_COOLING_DOWN:
    {
        cool_down_time.step(deltatime);
        if (cool_down_time.isTimeout())
        {
            state = SkillState::IS_AVAILABLE;
            cool_down_time.reset();
        }
    }
    default:
        break;
    }
}