// Урок 03: объект хранит своё состояние и контролирует его изменение.
#pragma once

namespace lesson {

class Target
{
public:
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

    bool isAlive() const
    {
        return health > 0;
    }

private:
    int health = 100;
};

} // namespace lesson
