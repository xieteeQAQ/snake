#include "gaobject.hpp"
#include "snake.hpp"

void PlayerData::hurt(int amount, Resources &res)
{
    int remainingAmount = amount;
    if (extraHealth)
    {
        if (extraHealth - amount < 0)
        {
            remainingAmount = remainingAmount - extraHealth;
            extraHealth = 0;
        }
        else
        {
            extraHealth -= amount;
            remainingAmount = 0;
        }
    }
    if (remainingAmount > 0)
    {
        currentHealth = currentHealth - remainingAmount < 0 ? 0 : currentHealth - remainingAmount;
    }
    playSound(res.groups[GROUP_INDEX_PLAYERHURT], -1);
}

void PlayerData::treat(int amount, Resources &res)
{
    currentHealth = currentHealth + amount > baseHealth ? baseHealth : currentHealth + amount;
    playSound(res.heal_sound);
}

void PlayerData::increaseExtraHealth(int amount)
{
    extraHealth += amount;
}

void PlayerData::increaseBaseHealth(int amount)
{
    baseHealth += amount;
}

void PlayerData::consumeBody(GameState &gs, Resources &res, int amount)
{
    int treatAmount = 5;
    int baseHealthAmount = 5;
    auto &v = gs.layers[LAYER_IDX_BODY];

    if (v.empty())
        return;
    if (v.size() < amount)
        amount = v.size();
    if (currentHealth + treatAmount * amount > baseHealth - baseHealthAmount * amount)
        amount -= amount - (baseHealth - currentHealth) / (baseHealthAmount + treatAmount);
    if (amount <= 0)
        return;

    baseHealth -= amount * baseHealthAmount;
    v.erase(v.end() - amount, v.end());
    if (v.empty())
    {
        gs.player().data.player.points.clear();
    }
    else
    {
        v.back().data.body.points.clear();
    }
    gs.player().data.player.treat(treatAmount * amount, res);
}

