#include "snake.hpp"

/*
    x (443, 1732)
    y (159, 988)
*/
void checkPointEdge(glm::vec2 &point)
{
    if (point.x <= 443)
        point.x += 3;
    else if (point.x >= 1732)
        point.x -= 3;
    if (point.y <= 159)
        point.y += 3;
    else if (point.y >= 988)
        point.y -= 3;
}

constexpr float TILE_SIZE = 32;
void drawObject(const State &state, GameState &gs, GameObject &obj, float deltaTime)
{
    if (!obj.tex)
        std::cerr << "null tex\n";
    const float spriteSize = 128;
    SDL_FRect src{.x = 0, .y = 0, .w = spriteSize, .h = spriteSize};
    SDL_FRect dst{.x = obj.position.x - gs.mapViewport.x, .y = obj.position.y - gs.mapViewport.y, .w = TILE_SIZE, .h = TILE_SIZE};
    SDL_FPoint cen{.x = TILE_SIZE / 2, .y = TILE_SIZE / 2};

    SDL_FlipMode flipMode;
    if (obj.type == ObjectType::player || obj.type == ObjectType::body)
        flipMode = obj.directionX == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    else
        flipMode = SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state._renderer, obj.tex, &src, &dst, obj.angle, &cen, flipMode);

    if (debug && collision_box)
    {
        SDL_FRect collisionBox = {.x = obj.position.x + obj.collider.x - gs.mapViewport.x, .y = obj.position.y + obj.collider.y - gs.mapViewport.y, .w = obj.collider.w, .h = obj.collider.h};
        SDL_SetRenderDrawBlendMode(state._renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(state._renderer, 225, 165, 0, 150);
        SDL_RenderFillRect(state._renderer, &collisionBox);
        SDL_SetRenderDrawBlendMode(state._renderer, SDL_BLENDMODE_NONE);
    }
}

void createBody(const State &state, GameState &gs, Resources &res)
{
    GameObject pre;
    if (gs.layers[LAYER_IDX_BODY].empty())
    {
        pre = gs.player();
    }
    else
    {
        pre = gs.lastBody();
    }

    if (pre.type == ObjectType::nullobj)
    {
        std::cerr << "create body error\n";
        return;
    }

    float distance = 1;
    float len = std::sqrtf(pre.velocity.x * pre.velocity.x + pre.velocity.y * pre.velocity.y);
    GameObject body;
    body.setType(ObjectType::body);
    body.tex = res.body;
    body.position.x = len > 0 ? pre.position.x - (pre.velocity.x / len) * distance : pre.position.x - pre.directionX * distance;
    body.position.y = len > 0 ? pre.position.y - (pre.velocity.y / len) * distance : pre.position.y - pre.directionY * distance;
    checkPointEdge(body.position);
    body.acceleration = glm::vec2{1800, 1800};
    body.maxSpeedX = 1800;
    body.maxSpeedY = 1800;
    body.collider = {.x = 9, .y = 8, .w = 14, .h = 16};
    body.data.body.number = gs.layers[LAYER_IDX_BODY].size();

    gs.layers[LAYER_IDX_BODY].push_back(body);
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
        case ObjectType::food:
        {
            gs.player().data.player.grow_counter.step(1);
            if (gs.player().data.player.grow_counter.isOver())
            {
                gs.player().data.player.grow_counter.reset();
                createBody(state, gs, res);
            }

            auto &v = gs.layers[LAYER_IDX_FOOD];
            int aim = -1;
            int pre_size = v.size();
            for (int i = 0; i < v.size(); ++i)
            {
                if (v[i].data.food.number == objB.data.food.number)
                {
                    aim = i;
                    break;
                }
            }
            if (aim != -1)
                v.erase(v.begin() + aim);
            gs.eat += pre_size - v.size();

            playSound(res.tracks[GROUP_INDEX_EAT], -1);
            break;
        }
        default:
            break;
        }
    }
    else if (objA.type == ObjectType::body)
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
                if (!currentDirectionX || (currentDirectionX > 0 && obj.velocity.x < 0) || (currentDirectionX < 0 && obj.velocity.x > 0))
                {
                    const float factorX = obj.velocity.x > 0 ? -1.5f : 1.5f;
                    float amountX = obj.acceleration.x * factorX * deltaTime;
                    obj.velocity.x = std::fabsf(obj.velocity.x) < std::fabsf(amountX) ? 0 : obj.velocity.x + amountX;
                }
                if (!currentDirectionY || (currentDirectionY > 0 && obj.velocity.y < 0) || (currentDirectionY < 0 && obj.velocity.y > 0))
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

        obj.velocity.x += currentDirectionX * obj.acceleration.x * deltaTime;
        obj.velocity.x = std::fabsf(obj.velocity.x) > obj.maxSpeedX ? currentDirectionX * obj.maxSpeedX : obj.velocity.x;

        obj.velocity.y += currentDirectionY * obj.acceleration.y * deltaTime;
        obj.velocity.y = std::fabsf(obj.velocity.y) > obj.maxSpeedY ? currentDirectionY * obj.maxSpeedY : obj.velocity.y;

        obj.position += obj.velocity * deltaTime;

        if (!gs.layers[LAYER_IDX_BODY].empty())
        {
            float dx = obj.velocity.x * deltaTime;
            float dy = obj.velocity.y * deltaTime;
            float deltaDistance = std::sqrt(dx * dx + dy * dy);
            obj.data.player.ruler.step(deltaDistance);

            while (obj.data.player.ruler.isOver())
            {
                glm::vec2 point = {obj.position.x, obj.position.y};
                checkPointEdge(point);
                obj.data.player.ruler.consumeOver();
                obj.data.player.points.push_back(point);
            }
        }
    }

    if (obj.type == ObjectType::body)
    {
        if (gs.bodys_changed)
            gs.bodysSort();
        GameObject &pre = obj.data.body.number == 0 ? gs.player() : gs.body(obj.data.body.number - 1);

        const float interval = 20;
        float currentDirectionX = 0;
        float currentDirectionY = 0;

        float distanceX = pre.position.x - obj.position.x;
        float distanceY = pre.position.y - obj.position.y;
        float distance = std::sqrt(distanceX * distanceX + distanceY * distanceY);

        float p_distanceX = distanceX;
        float p_distanceY = distanceY;
        float p_distance = distance;

        if (pre.type == ObjectType::player)
        {
            auto &p = pre.data.player.points;
            while (!p.empty())
            {
                p_distanceX = p[0].x - obj.position.x;
                p_distanceY = p[0].y - obj.position.y;
                p_distance = std::sqrt(p_distanceX * p_distanceX + p_distanceY * p_distanceY);
                if (p_distance <= 5)
                {
                    p.pop_front();
                    continue;
                }
                if (p.empty())
                {
                    p_distance = distance;
                }
                break;
            }
        }
        else
        {
            auto &p = pre.data.body.points;
            while (!p.empty())
            {
                p_distanceX = p[0].x - obj.position.x;
                p_distanceY = p[0].y - obj.position.y;
                p_distance = std::sqrt(p_distanceX * p_distanceX + p_distanceY * p_distanceY);
                if (p_distance <= 5)
                {
                    p.pop_front();
                    continue;
                }
                if (p.empty())
                {
                    p_distanceX = distanceX;
                    p_distanceY = distanceY;
                    p_distance = distance;
                }
                break;
            }
        }

        if (p_distanceX > 0)
            currentDirectionX += 1;
        else if (p_distanceX < 0)
            currentDirectionX -= 1;

        if (p_distanceY > 0)
            currentDirectionY += 1;
        else if (p_distanceY < 0)
            currentDirectionY -= 1;

        obj.directionX = currentDirectionX ? currentDirectionX : obj.directionX;
        obj.directionY = currentDirectionY ? currentDirectionY : obj.directionY;

        if (currentDirectionX || currentDirectionY)
        {
            if (!currentDirectionX || (currentDirectionX > 0 && obj.velocity.x < 0) || (currentDirectionX < 0 && obj.velocity.x > 0))
            {
                obj.velocity.x = 0;
            }
            if (!currentDirectionY || (currentDirectionY > 0 && obj.velocity.y < 0) || (currentDirectionY < 0 && obj.velocity.y > 0))
            {
                obj.velocity.y = 0;
            }
        }

        if (distance <= interval)
        {
            obj.velocity.x = 0;
            obj.velocity.y = 0;
        }
        else
        {
            obj.velocity.x += currentDirectionX * obj.acceleration.x * deltaTime;
            obj.velocity.x = std::fabsf(obj.velocity.x) > obj.maxSpeedX ? currentDirectionX * obj.maxSpeedX : obj.velocity.x;

            obj.velocity.y += currentDirectionY * obj.acceleration.y * deltaTime;
            obj.velocity.y = std::fabsf(obj.velocity.y) > obj.maxSpeedY ? currentDirectionY * obj.maxSpeedY : obj.velocity.y;
        }

        float dx = obj.velocity.x * deltaTime;
        float dy = obj.velocity.y * deltaTime;
        obj.position += obj.velocity * deltaTime;

        if (obj.data.body.number != gs.lastBody().data.body.number)
        {
            float deltaDistance = std::sqrt(dx * dx + dy * dy);
            obj.data.body.ruler.step(deltaDistance);

            while (obj.data.body.ruler.isOver())
            {
                glm::vec2 point = {obj.position.x, obj.position.y};
                checkPointEdge(point);
                obj.data.body.ruler.consumeOver();
                obj.data.body.points.push_back(point);
            }
        }
    }

    for (auto &layer : gs.layers)
    {
        for (GameObject &objB : layer)
        {
            checkCollision(state, gs, res, obj, objB, deltaTime);
        }
    }
}

constexpr int MAP_ROWS = 48;
constexpr int MAP_COLS = 60;
void createMap(const State &state, GameState &gs, const Resources &res)
{
    short map[MAP_ROWS][MAP_COLS] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    const auto createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type)
    {
        GameObject o;
        o.setType(type);
        o.position = glm::vec2(c * TILE_SIZE, r * TILE_SIZE);
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
            case 2:
            {
                GameObject player = createObject(r, c, res.tex_standby, ObjectType::player);
                player.acceleration = glm::vec2(200, 200);
                player.maxSpeedX = 125;
                player.maxSpeedY = 125;
                player.collider = {.x = 5, .y = 0, .w = 23, .h = 31};
                gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
                gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
                break;
            }
            case 1:
            {
                GameObject QAQ = createObject(r, c, res.QAQ, ObjectType::level);
                gs.layers[LAYER_IDX_LEVEL].push_back(QAQ);
                break;
            }
            default:
                break;
            }
        }
    }
    assert(gs.playerIndex != -1);
}

void handleKayInput(const State &state, GameState &gs, GameObject &obj, Resources &res, float deltatime,
                    SDL_Scancode key, bool keydown)
{
    if (obj.type == ObjectType::player)
    {
        const float running_force = 200;
        switch (key)
        {
        case SDL_SCANCODE_F:
        {
            static bool keyup = true;
            if (obj.data.player.state == PlayerState::running && keydown && keyup)
            {
                keyup = false;
                if (obj.data.player.skills.skill_sprint.state == SkillState::IS_AVAILABLE)
                {
                    playSound(res.tracks[GROUP_INDEX_SPRING], -1); 
                }
                obj.data.player.skills.sprint();
            }
            if (!keydown)
                keyup = true;
            break;
        }
        case SDL_SCANCODE_1:
        {
            if (keydown)
                debug = !debug;
            break;
        }
        case SDL_SCANCODE_2:
        {
            if (keydown && debug)
                collision_box = !collision_box;
            break;
        }
        default:
            break;
        }
    }
}

void drawBackground(State &state, GameState &gs, GameObject &obj, SDL_Texture *tex)
{
    if (obj.type == ObjectType::player)
    {
        const float backgroundW = 1000;
        const float backgroundH = 429;
        const float size = 2.5;
        SDL_FRect src = {.x = 0, .y = 0, .w = backgroundW, .h = backgroundH};
        SDL_FRect dst = {.x = -gs.mapViewport.x, .y = -gs.mapViewport.y, .w = backgroundW * size, .h = backgroundH * size};
        SDL_RenderTextureRotated(state._renderer, tex, &src, &dst, 0, nullptr, SDL_FLIP_NONE);
    }
}

void generateFood(State &state, GameState &gs, Resources &res, float deltaTime)
{
    static Timer interval(1.5);

    if (gs.layers[LAYER_IDX_FOOD].size() >= 50)
        return;

    if (interval.isTimeout())
    {
        interval.reset();
    }
    else
    {
        interval.step(deltaTime);
        return;
    }

    std::mt19937 generater(rd());
    /*
        x (443, 1732)
        y (159, 988)
    */
    std::normal_distribution<float> distributionX(1088.0f, 400.0f);
    std::normal_distribution<float> distributionY(576.5f, 400.0f);
    ++gs.food_count;

    float x = 0, y = 0;
    while (x <= 440 || x >= 1736)
    {
        x = distributionX(generater);
    }
    while (y <= 156 || y >= 998)
    {
        y = distributionY(generater);
    }

    GameObject food;
    food.setType(ObjectType::food);
    food.tex = res.food;
    food.position = glm::vec2(x, y);
    food.collider = {.x = 8, .y = 8.5, .w = 14, .h = 13.5};
    food.data.food.number = gs.food_count;

    gs.layers[LAYER_IDX_FOOD].push_back(food);
}

void writeDebugText(State &state, GameState &gs, float deltaTime)
{
    SDL_SetRenderDrawColor(state._renderer, 0, 0, 0, 255);
    SDL_FRect rect = {.x = 5.5, .y = 3.5, .w = 44, .h = 12.5};
    SDL_RenderFillRect(state._renderer, &rect);
    rect = {.x = 52.5, .y = 3.5, .w = 99, .h = 12.5};
    SDL_RenderFillRect(state._renderer, &rect);
    SDL_SetRenderDrawColor(state._renderer, 225, 60, 0, 255);
    SDL_RenderDebugText(state._renderer, 8, 6, "DEBUG");

    if (collision_box)
        SDL_SetRenderDrawColor(state._renderer, 225, 165, 0, 255);
    else
        SDL_SetRenderDrawColor(state._renderer, 90, 90, 90, 255);
    SDL_RenderDebugText(state._renderer, 55, 6, "COLLISIONBOX");

    SDL_SetRenderDrawColor(state._renderer, 0, 0, 0, 255);
    std::stringstream debug_1;
    static Timer present_fps(3);
    static long long cur_fps = 1 / deltaTime;
    if (present_fps.isTimeout())
    {
        present_fps.reset();
        cur_fps = 1 / deltaTime;
    }
    else
    {
        present_fps.step(deltaTime);
    }
    debug_1 << "fps: " << static_cast<long long>(cur_fps <= 999 ? cur_fps : 999) << (cur_fps <= 999 ? "" : "+");

    std::stringstream debug_2;
    debug_2 << std::setiosflags(std::ios::fixed) << std::setprecision(2) << "x: " << gs.player().position.x << ", y: " << gs.player().position.y;

    std::stringstream debug_3;
    debug_3 << "food_count: " << gs.food_count << ", eat: " << gs.eat << " , food_vec: " << gs.layers[LAYER_IDX_FOOD].size();

    std::stringstream debug_4;
    debug_4 << "bodys_amount: " << gs.layers[LAYER_IDX_BODY].size() << ", grow: " << gs.player().data.player.grow_counter.getNumber() << "/" << gs.player().data.player.grow_counter.getLength();

    SDL_RenderDebugText(state._renderer, 7, 20, debug_1.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 40, debug_2.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 60, debug_3.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 80, debug_4.str().c_str());

    SDL_SetRenderDrawColor(state._renderer, 30, 30, 30, 255);
}

void playBGM(MIX_Track *track)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_SetTrackGain(track, 0.3f); // 30% 音量
    MIX_PlayTrack(track, props);
}

void playSound(MIX_Track *track)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, 0);
    MIX_SetTrackGain(track, 0.3f); // 30% 音量
    MIX_PlayTrack(track, props);
}

// -1: play randomly 
void playSound(std::vector<MIX_Track*> &group, int index)
{
    if (index >= static_cast<int>(group.size()))
    {
        std::cerr << "playSound: wrong index";
        return;
    }
    else if (index < 0)
    {
        std::mt19937 generater(rd());
        std::uniform_int_distribution<int> distribution(0, group.size() - 1);
        index = distribution(generater);
    }

    playSound(group[index]);
}