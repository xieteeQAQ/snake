#include "snake.hpp"

/*
    x (443, 1732)
    y (159, 988)
*/
void checkPointEdge(glm::vec2 &point)
{
    if (point.x <= LEFTEDGE + 1)
    {
        point.x += LEFTEDGE + 1 - point.x;
    }
    else if (point.x >= RIGHTEDGE - 1)
    {
        point.x -=  point.x - (RIGHTEDGE - 1);
    }
    if (point.y <= UPPEREDGE + 1)
    {
        point.y += UPPEREDGE + 1 - point.y;
    }
    else if (point.y >= LOWERLEFTEDGE - 1)
    {
        point.y -= point.x - (LOWERLEFTEDGE - 1);
    }
}

constexpr float TILE_SIZE = 32;
void drawObject(const State &state, GameState &gs, GameObject &obj, float deltaTime)
{
    if (obj.type == ObjectType::bullet && obj.data.bullet.state == BulletState::inactive)
        return;

    const float spriteSize = 128;
    float srcX = obj.currentAnimation != -1 ? obj.animation[obj.currentAnimation].currentFrame() * spriteSize : 0.0f;
    float screenX = obj.position.x - gs.mapViewport.x;
    float screenY = obj.position.y - gs.mapViewport.y;
    SDL_FRect src{.x = srcX, .y = 0, .w = spriteSize, .h = spriteSize};
    SDL_FRect dst{.x = screenX, .y = screenY, .w = TILE_SIZE, .h = TILE_SIZE};
    SDL_FPoint cen{.x = TILE_SIZE / 2, .y = TILE_SIZE / 2};

    SDL_FlipMode flipMode;
    if (obj.type == ObjectType::player || obj.type == ObjectType::body)
        flipMode = obj.directionX == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    else
        flipMode = SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state._renderer, obj.tex, &src, &dst, obj.angle, &cen, flipMode);

    if (debug && collision_box && obj.collideable)
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

    float distance = 24;
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
                int baseHealthAmount = 5;
                int extraHealthAmount = 5;
                gs.player().data.player.grow_counter.reset();
                gs.player().data.player.increaseBaseHealth(baseHealthAmount);
                gs.player().data.player.increaseExtraHealth(extraHealthAmount);
                createBody(state, gs, res);
                playSound(res.burp);
            }
            else
            {
                playSound(res.groups[GROUP_INDEX_EAT], -1);
            }

            // find the food object and delect it
            auto &v = gs.layers[LAYER_IDX_FOOD];
            for (int i = 0; i < v.size(); ++i)
            {
                if (v[i].data.food.number == objB.data.food.number)
                {
                    v[i].collideable = false;
                    v[i].tex = nullptr;
                    gs.eat += 1;
                    break;
                }
            }
            break;
        }
        case ObjectType::bullet:
        {
            switch (objB.data.bullet.type)
            {
            case BulletType::potatoMine:
            {
                // only trigger explosion when mine is armed (idle)
                if (objB.data.bullet.state == BulletState::idle)
                {
                    objB.tex = res.potato_boom;
                    objB.collideable = false;
                    objB.data.bullet.timer.reset();
                    objB.data.bullet.timer.setLength(3); // the "boom"s duration is 3s
                    objB.currentAnimation = -1;
                    objB.data.bullet.state = BulletState::colliding;
                    gs.potato_count -= 1;

                    objA.data.player.hurt(objB.data.bullet.attack, res);
                    playSound(res.potato_boom_sound);
                }
                break;
            }
            case BulletType::Frying:
            {
                if (objB.data.bullet.state == BulletState::moving)
                {
                    objB.tex = nullptr;
                    objB.collideable = false;
                    objB.data.bullet.state = BulletState::inactive;

                    objA.data.player.hurt(objB.data.bullet.attack, res);
                }
            }
            default:
                break;
            }
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
    if (!a.collideable || !b.collideable)
        return;
    if (a.type == b.type)
        return;
    if (!(a.type == player || b.type == player))
    {
        if ((a.type == level && b.type == food) || (a.type == food && b.type == level))
            return;
        if ((a.type == food && b.type == body) || (a.type == body && b.type == food))
            return;
        if ((a.type == bullet && b.type == level) || (a.type == level && b.type == bullet))
            return;
    }
    else if (a.type == body || b.type == body)
        return;

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
        updatePlayer(state, gs, res, obj, deltaTime);
    }

    if (obj.type == ObjectType::body)
    {
        updateBody(state, gs, res, obj, deltaTime);
    }

    if (obj.type == ObjectType::bullet)
    {
       updateBullet(state, gs, res, obj, deltaTime);
    }

    for (auto &layer : gs.layers)
    {
        for (GameObject &objB : layer)
        {
            checkCollision(state, gs, res, obj, objB, deltaTime);
        }
    }

    for (auto &bullet : gs.bullets)
    {
        for (auto &b : bullet)
        {
            if (b.data.bullet.type == BulletType::Frying)
            {
                if (outOfRange(b))
                {
                    b.collideable = false;
                    b.tex = nullptr;
                    b.data.bullet.state = BulletState::inactive;
                    continue;
                }
            }
            checkCollision(state, gs, res, obj, b, deltaTime);
        }
    }

    for (int i = BULLET_IDX_FRYING; i < gs.bullets.size(); ++i)
    {
        gs.bullets[i].erase(std::remove_if(gs.bullets[i].begin(), gs.bullets[i].end(), [](GameObject &b){
            return b.data.bullet.state == BulletState::inactive;
        }), gs.bullets[i].end());
    }
}

void updatePlayer(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
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

    if (obj.data.player.currentHealth + obj.data.player.extraHealth > obj.data.player.baseHealth)
    {

        obj.data.player.totalHealth = obj.data.player.baseHealth + obj.data.player.extraHealth;
    }
    else
    {
        obj.data.player.totalHealth = obj.data.player.baseHealth;
    }

    edgeDetection(state, gs, obj);
}

void updateBody(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
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

    edgeDetection(state, gs, obj);
}

void updateBullet(const State &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
{
     switch (obj.data.bullet.type)
    {
    case BulletType::potatoMine:
    {

        if (obj.tex == res.potato_boom)
        {
            if (obj.data.bullet.timer.isTimeout())
            {
                obj.tex = nullptr;
                obj.data.bullet.state = BulletState::inactive;
            }
            else
            {
                obj.data.bullet.timer.step(deltaTime);
            }
            break;
        }
        else if (obj.currentAnimation == res.ANIM_POTATO_IDLE)
        {
            break;
        }
        else if (obj.currentAnimation == res.ANIM_POTATO_GROW)
        {
            if (obj.animation[obj.currentAnimation].isDone())
            {
                obj.currentAnimation = res.ANIM_POTATO_IDLE;
                obj.tex = res.potato_2;
                obj.collider = SDL_FRect{.x = 5, .y = 14, .w = 23, .h = 12};
                obj.collideable = true;
                if (obj.position.x - gs.mapViewport.x > 0 && obj.position.x - gs.mapViewport.x < state.logW &&
                    obj.position.y - gs.mapViewport.y > 0 && obj.position.y - gs.mapViewport.y < state.logH)
                {
                    playSound(res.plant_rise, 0.5);
                }
            }
        }
        else if (obj.data.bullet.timer.isTimeout())
        {
            obj.tex = res.potato_1;
            obj.currentAnimation = res.ANIM_POTATO_GROW;
        }
        else
        {
            obj.data.bullet.timer.step(deltaTime);
        }
        break;
    }
    case BulletType::Frying:
    {
        obj.position.x += std::sinf(obj.angle) * obj.velocity.x;
        obj.position.y += std::cosf(obj.angle) * obj.velocity.y;
    }
    default:
        break;
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
                player.data.player.currentHealth = player.data.player.totalHealth = player.data.player.baseHealth = 100;
                player.data.player.extraHealth = 0;
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
                    playSound(res.groups[GROUP_INDEX_SPRING], -1);
                }
                obj.data.player.skills.sprint();
            }
            if (!keydown)
                keyup = true;
            break;
        }
        case SDL_SCANCODE_G:
        {
            static bool keyup = true;
            if (keydown && keyup)
            {
                keyup = false;
                gs.player().data.player.consumeBody(gs, res);
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

    if (gs.food_count - gs.eat >= 50)
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
    std::normal_distribution<float> distributionX(1088.0f, 214.0f);
    std::normal_distribution<float> distributionY(576.5f, 138.0f);
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

    bool findAlternative = false;
    for (auto &f : gs.layers[LAYER_IDX_FOOD])
    {
        if (f.collideable == false)
        {
            findAlternative = true;
            f = food;
            f.data.food.number = food.data.food.number;
            break;
        }
    }
    if (!findAlternative)
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
    debug_1 << "fps: " << static_cast<long long>(cur_fps <= 999 ? cur_fps : 999) << (cur_fps <= 999 ? "" : "+")
            << std::setiosflags(std::ios::fixed) << std::setprecision(2) << ", time: " << static_cast<int>(playtime);

    std::stringstream debug_2;
    debug_2 << std::setiosflags(std::ios::fixed) << std::setprecision(2) << "x: " << gs.player().position.x << ", y: " << gs.player().position.y;

    std::stringstream debug_3;
    debug_3 << "food_count: " << gs.food_count << ", eat: " << gs.eat << " , food_vec: " << gs.layers[LAYER_IDX_FOOD].size();

    std::stringstream debug_4;
    debug_4 << "bodys_amount: " << gs.layers[LAYER_IDX_BODY].size() << ", grow: " << gs.player().data.player.grow_counter.getNumber() << "/" << gs.player().data.player.grow_counter.getLength();

    std::stringstream debug_5;
    debug_5 << "baseHP: " << gs.player().data.player.baseHealth << ", curHP: " << gs.player().data.player.currentHealth << ", extraHP: " << gs.player().data.player.extraHealth;

    std::stringstream debug_6;
    debug_6 << "bullet: " << gs.bullets[BULLET_IDX_FRYING].size();

    SDL_RenderDebugText(state._renderer, 7, 20, debug_1.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 40, debug_2.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 60, debug_3.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 80, debug_4.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 100, debug_5.str().c_str());
    SDL_RenderDebugText(state._renderer, 7, 120, debug_6.str().c_str());

    SDL_SetRenderDrawColor(state._renderer, 30, 30, 30, 255);
}

void playBGM(MIX_Track *track, float volume)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_SetTrackGain(track, volume); // 30% 音量
    MIX_PlayTrack(track, props);
}

void playSound(MIX_Track *track, float volume)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, 0);
    MIX_SetTrackGain(track, 0.3f); // 30% 音量
    MIX_PlayTrack(track, props);
}

// -1: play randomly
void playSound(std::vector<MIX_Track *> &groups, int index, float volume)
{
    if (index >= static_cast<int>(groups.size()))
    {
        std::cerr << "playSound: wrong index";
        return;
    }
    else if (index < 0)
    {
        std::mt19937 generater(rd());
        std::uniform_int_distribution<int> distribution(0, groups.size() - 1);
        index = distribution(generater);
    }

    playSound(groups[index]);
}

void generatePotatoMine(State &state, GameState &gs, Resources &res, float deltaTime)
{
    static Timer interval(10);

    if (gs.potato_count >= 10)
        return;

    if (interval.isTimeout())
    {
        interval.reset();
        gs.potato_count += 1;
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
    std::normal_distribution<float> distributionX(1088.0f, 214.0f);
    std::normal_distribution<float> distributionY(576.5f, 138.0f);

    float x = distributionX(generater);
    float y = distributionY(generater);
    bool inRange = false;
    while (!inRange)
    {
        if (x <= 443 + 150 || x >= 1732 - 150)
        {
            x = distributionX(generater);
            continue;
        }
        if (y <= 159 + 150 || y >= 988 - 150)
        {
            y = distributionY(generater);
            continue;
        }
        inRange = true;
    }

    GameObject potato;
    potato.setType(ObjectType::bullet);
    potato.animation = res.potatoAnims;
    potato.tex = res.potato_0;
    potato.currentAnimation = -1;
    potato.position = glm::vec2(x, y);
    potato.collideable = false;
    potato.data.bullet.state = BulletState::idle;
    potato.data.bullet.type = BulletType::potatoMine;
    potato.data.bullet.attack = 50;
    potato.data.bullet.timer.setLength(15);
    potato.data.bullet.timer.setTimeout(false);

    bool findAlternative = false;
    for (auto &p : gs.bullets[BULLET_IDX_POTATO])
    {
        if (p.data.bullet.state == BulletState::inactive)
        {
            findAlternative = true;
            p = potato;
            p.data.bullet.timer.reset();
            p.data.bullet.timer.setLength(15);
            p.data.bullet.timer.setTimeout(false);
            potato.data.bullet.attack = 10;
            break;
        }
    }
    if (!findAlternative)
    {
        potato.data.bullet.number = gs.bullets[BULLET_IDX_POTATO].size();
        gs.bullets[BULLET_IDX_POTATO].push_back(potato);
    }

    if (potato.position.x - gs.mapViewport.x > 0 && potato.position.x - gs.mapViewport.x < state.logW &&
        potato.position.y - gs.mapViewport.y > 0 && potato.position.y - gs.mapViewport.y < state.logH)
    {
        playSound(res.planting_sound);
    }
}

void drawUI(State &state, GameState &gs)
{
    drawPlayerHealth(state, gs);
}

void drawPlayerHealth(State &state, GameState &gs)
{
    const auto &data = gs.player().data.player;
    float baseLength = static_cast<float>(data.baseHealth);
    float extraLength = data.extraHealth < data.baseHealth ? static_cast<float>(data.extraHealth) : static_cast<float>(data.baseHealth);
    float currentLength = static_cast<float>(data.currentHealth);
    float totalLength = static_cast<float>(data.totalHealth);
    
    const float baseX = 15.0f;
    const float baseY = static_cast<float>(state.logH) - 30.0f;
    const float height = 15.0f;
    const float baseY_health_amount = 60.0f;

    SDL_FRect baseRect = {
        .x = baseX,
        .y = baseY,
        .w = baseLength,
        .h = height
    };

    SDL_SetRenderDrawColor(state._renderer, 178, 34, 34, 255);
    SDL_RenderFillRect(state._renderer, &baseRect);

    if (gs.player().data.player.currentHealth > 0)
    {
        SDL_FRect currentRect = {
            .x = baseX,
            .y = baseY,
            .w = currentLength,
            .h = height
        };
        SDL_SetRenderDrawColor(state._renderer, 225, 225, 0, 255);
        SDL_RenderFillRect(state._renderer, &currentRect);
    }

    if (gs.player().data.player.extraHealth > 0)
    {
        SDL_FRect extraRect = {
            .x = baseX,
            .y = baseY,
            .w = extraLength,
            .h = height
        };
        SDL_SetRenderDrawBlendMode(state._renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(state._renderer, 0, 225, 225, 210);
        SDL_RenderFillRect(state._renderer, &extraRect);
        SDL_SetRenderDrawBlendMode(state._renderer, SDL_BLENDMODE_NONE);
    }

    std::stringstream text;
    text << "HP: " << gs.player().data.player.currentHealth;
    if (gs.player().data.player.extraHealth > 0)
    {
      text << " + " << gs.player().data.player.extraHealth;
    }
    text << " / " << gs.player().data.player.baseHealth;
    TTF_TextEngine *engine = TTF_CreateRendererTextEngine(state._renderer);
    TTF_Font *font = TTF_OpenFont("./font/Impact.ttf", 16.0f);
    if (!font)
    {
        SDL_Log("Font load failed: %s", SDL_GetError());
    }
    TTF_Text *health_amount = TTF_CreateText(engine, font, text.str().c_str(), 0);
    TTF_DrawRendererText(health_amount, baseX, state.logH - baseY_health_amount);
}

void edgeDetection(const State &state, GameState &gs, GameObject &obj)
{
    /*
        x (443, 1732)
        y (159, 988)
    */
    bool OutOfRangeX = obj.position.x < LEFTEDGE || obj.position.x > RIGHTEDGE;
    bool OutOfRangeY = obj.position.y < UPPEREDGE || obj.position.y > LOWERLEFTEDGE;
    if (OutOfRangeX || OutOfRangeY)
    {
        switch (obj.type)
        {
            case ObjectType::player:
            {
                checkPointEdge(obj.position);
            }
            case ObjectType::body:
            {
            auto &bodys = gs.layers[LAYER_IDX_BODY];
            for (int i = 0; i < bodys.size(); ++i)
            {
                if (bodys[i].data.body.number == obj.data.body.number)
                {
                    GameObject &pre = i == 0 ? gs.player() : bodys[i - 1];
                    auto &points = i == 0 ? pre.data.player.points : pre.data.body.points;
                    float distance = 24;
                    float len = std::sqrtf(pre.velocity.x * pre.velocity.x + pre.velocity.y * pre.velocity.y);
                    obj.position.x = len > 0 ? pre.position.x - (pre.velocity.x / len) * distance : pre.position.x - pre.directionX * distance;
                    obj.position.y = len > 0 ? pre.position.y - (pre.velocity.y / len) * distance : pre.position.y - pre.directionY * distance;
                    checkPointEdge(obj.position);
                    points.clear();
                }
            }
            break;
            }
        default:
            break;
        }
    }
}

void updateMapViewPort(State &state, GameState &gs, GameObject &obj, float deltatime)
{
    const float thresholdX = gs.mapViewport.w * 0.05f;
    const float thresholdY = gs.mapViewport.h * 0.05f;
    float playerCenterX = gs.player().position.x + 32.0f;
    float playerCenterY = gs.player().position.y + 32.0f;
    float viewCenterX = gs.mapViewport.x + gs.mapViewport.w / 2.0f;
    float viewCenterY = gs.mapViewport.y + gs.mapViewport.h / 2.0f;
    float dx = playerCenterX - viewCenterX;
    float dy = playerCenterY - viewCenterY;
    if (std::fabsf(dx) > thresholdX)
    {
        // gs.mapViewport.x += dx - std::copysign(thresholdX, dx);
        gs.mapViewport.x += (dx - std::copysign(thresholdX, dx)) * (40.0f / static_cast<float>(state.width));
    }

    if (std::fabsf(dy) > thresholdY)
    {
        gs.mapViewport.y += (dy - std::copysign(thresholdY, dy)) * (40.0f / static_cast<float>(state.height));
    }
}

void createCircleBullet(State &state, GameState &gs, Resources &res, SDL_Texture *tex, const float &x, const float &y, glm::vec2 velocity, SDL_FRect collider, int attack, int amount, float deltatime)
{
    for (int i = 0; i < amount; ++i)
    {
        GameObject bullet;
        bullet.setType(ObjectType::bullet);
        bullet.tex = tex;
        bullet.collider = collider;
        bullet.position = glm::vec2(x, y);
        bullet.velocity = velocity;
        bullet.currentAnimation = -1;
        bullet.angle = 360.0f / static_cast<float>(i + 1);
        bullet.data.bullet.attack = attack;
        bullet.data.bullet.state = BulletState::moving;
        bullet.data.bullet.type = BulletType::Frying;
        gs.bullets[BULLET_IDX_FRYING].push_back(bullet);
    }
}

bool outOfRange(GameObject &obj)
{
    bool OutOfRangeX = obj.position.x < LEFTEDGE || obj.position.x > RIGHTEDGE;
    bool OutOfRangeY = obj.position.y < UPPEREDGE || obj.position.y > LOWERLEFTEDGE;
    return OutOfRangeX || OutOfRangeY;
}

void drawWarning(State &state, GameState &gs, Resources &res, glm::vec2 position)
{
    float screenX = position.x - gs.mapViewport.x;
    float screenY = position.y - gs.mapViewport.y;
    SDL_FRect src = {.x = 0, .y = 0, .w = 128, .h = 128};
    SDL_FRect dst = {.x = screenX, .y = screenY, .w = TILE_SIZE, .h = TILE_SIZE};
    SDL_FPoint cen = {.x = TILE_SIZE / 2, .y = TILE_SIZE / 2};
    SDL_RenderTextureRotated(state._renderer, res.warning, &src, &dst, 0, &cen, SDL_FLIP_NONE);
}