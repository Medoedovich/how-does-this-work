// Урок 04: конструктор задаёт начальное состояние каждого экземпляра.
#pragma once

namespace lesson {

class Target
{
public:
    // Если аргумент не указан, используем 100. Отрицательное здоровье обнуляем.
    Target(int startHealth = 100)
        : health(startHealth)
        , maximumHealth(startHealth)
    {
        if (health < 0)
        {
            health = 0;
            maximumHealth = 0;
        }
    }

    // Неположительный урон игнорируем. Здоровье не опускается ниже нуля.
    void takeDamage(int damage)
    {
        if (damage <= 0)
            return;

        health -= damage;

        if (health < 0)
            health = 0;
    }

    int getHealth() const
    {
        return health;
    }

    int getMaxHealth() const
    {
        return maximumHealth;
    }

    bool isAlive() const
    {
        return health > 0;
    }

private:
    int health;
    int maximumHealth;
};

} // namespace lesson
