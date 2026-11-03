// Учебный код. Математический тип Vec3 взят из OpenBF2 без изменений.
#pragma once

#include "original/Vec3.h"
#include <algorithm>
#include <cmath>

namespace lesson {

using dice::hfe::Vec3;

// Изменяем переданный объект. false означает: направление слишком маленькое.
inline bool normalize(Vec3& v)
{
    const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length < 0.000001f)
        return false;

    v.x /= length;
    v.y /= length;
    v.z /= length;
    return true;
}

// Отличается ровно отсутствием &: изменения остаются внутри локальной копии.
inline bool normalizeCopy(Vec3 v)
{
    return normalize(v);
}

struct World
{
    Vec3 player = {1.0f, 0.0f, 2.0f};
    Vec3 flag = {4.0f, 0.0f, 6.0f};
    Vec3 velocity = Vec3::getZero();
    Vec3 rawInput = Vec3::getZero();
    Vec3 direction = Vec3::getZero();
    Vec3 forward = {0.0f, 0.0f, 1.0f};
    float speed = 2.0f;
    float captureRadius = 4.0f;
    bool normalizeInput = true;
    bool passByValue = false;
    bool autoWalk = false;
};

// Оригинальная функция OpenBF2 считает квадрат расстояния на плоскости XZ.
inline float distanceToFlag(const World& world)
{
    return std::sqrt(dice::hfe::distXZSqr(world.player, world.flag));
}

inline bool inCaptureRadius(const World& world)
{
    return dice::hfe::distXZSqr(world.player, world.flag)
        <= world.captureRadius * world.captureRadius;
}

inline float actualSpeed(const World& world)
{
    return std::sqrt(world.velocity.x * world.velocity.x
                   + world.velocity.z * world.velocity.z);
}

// Главная учебная функция. renderer.cpp только рисует её результат.
// inputX / inputZ: -1, 0 или +1 от клавиш. dt: прошедшее время в секундах.
inline void updatePlayer(World& world, float inputX, float inputZ, float dt)
{
    world.rawInput = {inputX, 0.0f, inputZ};

    if (world.autoWalk)
    {
        world.rawInput.x = world.flag.x - world.player.x;
        world.rawInput.z = world.flag.z - world.player.z;
    }

    world.direction = world.rawInput;
    if (world.normalizeInput)
    {
        if (world.passByValue)
            normalizeCopy(world.direction);
        else
            normalize(world.direction);
    }

    world.velocity = {
        world.direction.x * world.speed,
        0.0f,
        world.direction.z * world.speed
    };

    // displacement = velocity * dt. Никаких перегруженных операторов не требуется.
    Vec3 displacement = {
        world.velocity.x * dt,
        0.0f,
        world.velocity.z * dt
    };

    // При автоматическом движении не пролетаем через саму точку флага.
    if (world.autoWalk)
    {
        const float stepSquared = displacement.x * displacement.x
                                + displacement.z * displacement.z;
        if (stepSquared >= dice::hfe::distXZSqr(world.player, world.flag))
        {
            world.player = world.flag;
            world.autoWalk = false;
            world.velocity = Vec3::getZero();
            return;
        }
    }

    const Vec3 previousPosition = world.player;
    world.player.x += displacement.x;
    world.player.z += displacement.z;

    // Граница учебной площадки. Это наше правило, а не логика Battlefield.
    world.player.x = std::max(-12.0f, std::min(12.0f, world.player.x));
    world.player.z = std::max(-12.0f, std::min(12.0f, world.player.z));

    // У стены отображаем скорость фактического перемещения.
    if (dt > 0.0f)
    {
        world.velocity.x = (world.player.x - previousPosition.x) / dt;
        world.velocity.z = (world.player.z - previousPosition.z) / dt;
    }

    Vec3 facing = world.velocity;
    if (normalize(facing))
        world.forward = facing;
}

} // namespace lesson
