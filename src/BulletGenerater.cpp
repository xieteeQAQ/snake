#include "BulletGenerater.hpp"

void updateBulletGenerater(const State &state, GameState &gs, Resources &res, float deltaTime)
{
    for (int i = 0; i < gs.bulletGeneraters.size(); ++i)
    {
        auto &bg = gs.bulletGeneraters[i];
        if (bg.isAlive() && bg.state != BulletGenerater::BgState::ready)
        {
            if (!bg.delayIsTimeOut())
            {
                bg.state = BulletGenerater::BgState::delaying;
                bg.stepDelayTime(deltaTime);
            }
            else
            {
                bg.state = BulletGenerater::BgState::warning;
                bg.stepWarningTime(deltaTime);
                if (bg.WaringIsTimeOut())
                {
                    bg.state = BulletGenerater::BgState::ready;
                    bg.resetWarningTime();
                    bg.resetDelayTime();
                    switch (bg.type)
                    {
                    case BulletGenerater::Type::circleStickyRice:
                    {
                        bg.state = BulletGenerater::BgState::waiting;
                        glm::vec2 velocity = {5.0f, 5.0f};
                        SDL_FRect collision = {.x = 13, .y = 10, .w = 6, .h = 12};
                        int attack = 10;
                        int amount = 10;
                        bg.createCircleBullet(state, gs, res, res.stickyRice, bg.position, velocity, collision, attack, amount);
                        bg.randomPosition();
                        break;
                    }
                    default:
                        break;
                    }
                }
                else
                {
                    drawWarning(state, gs, res, bg.position);
                }
            }
        }
    }
}

void BulletGenerater::putWarning(const State &state, GameState &gs)
{
    float screenX = position.x - gs.mapViewport.x;
    float screenY = position.y - gs.mapViewport.y;
    SDL_FRect src = {.x = 0, .y = 0, .w = 128, .h = 128};
    SDL_FRect dst = {.x = screenX, .y = screenY, .w = TILE_SIZE, .h = TILE_SIZE};
    SDL_FPoint cen = {.x = TILE_SIZE / 2, .y = TILE_SIZE / 2};
    SDL_RenderTextureRotated(state._renderer, warningTex, &src, &dst, 0, &cen, SDL_FLIP_NONE);
}

void BulletGenerater::createCircleBullet(const State &state, GameState &gs, Resources &res, SDL_Texture *tex, const glm::vec2 &position,glm::vec2 velocity, SDL_FRect collider, int attack, int amount)
{
    for (int i = 0; i < amount; ++i)
    {
        GameObject bullet;
        bullet.setType(ObjectType::bullet);
        bullet.tex = tex;
        bullet.collider = collider;
        bullet.position = position;
        bullet.velocity = velocity;
        bullet.animation = res.bulletAnims;
        bullet.currentAnimation = res.ANIM_STICKYRICE_SPIN;
        bullet.angle = 360.0f / static_cast<float>(i + 1);
        bullet.data.bullet.attack = attack;
        bullet.data.bullet.state = BulletState::moving;
        bullet.data.bullet.type = BulletType::Frying;
        gs.bullets[BULLET_IDX_FRYING].push_back(bullet);
    }
}

void BulletGenerater::randomPosition()
{
    std::mt19937 generater(rd());
    std::uniform_int_distribution<int> distX(LEFTEDGE, RIGHTEDGE);
    std::uniform_int_distribution<int> distY(UPPEREDGE, LOWERLEFTEDGE);
    position.x = static_cast<float>(distX(generater));
    position.y = static_cast<float>(distY(generater));
}