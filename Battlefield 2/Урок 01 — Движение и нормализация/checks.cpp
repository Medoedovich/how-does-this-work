#include "lesson.h"
#include <iostream>

int main()
{
    int failed = 0;
    int count = 0;
    auto near = [](float a, float b) { return std::fabs(a - b) < 0.0001f; };
    auto check = [&](bool condition, const char* name) {
        ++count;
        if (!condition) ++failed;
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
    };
    using namespace lesson;
    World world;
    check(near(distanceToFlag(world), 5), "original distXZSqr: 3-4-5 distance");
    check(!inCaptureRadius(world), "five metres is outside four metre radius");
    world.captureRadius = 5;
    check(inCaptureRadius(world), "capture includes the radius boundary");
    Vec3 vector{6, 0, 8};
    check(normalizeCopy(vector) && near(vector.x, 6) && near(vector.z, 8), "pass by value preserves caller");
    check(normalize(vector) && near(vector.x, .6f) && near(vector.z, .8f), "pass by reference changes caller");
    Vec3 zero = Vec3::getZero();
    check(!normalize(zero) && near(zero.x, 0), "zero direction cannot be normalized");
    world = World{};
    updatePlayer(world, 1, 1, .5f);
    check(near(actualSpeed(world), 2), "normalized diagonal keeps chosen speed");
    check(near(world.player.x, 1 + std::sqrt(.5f)) && near(world.player.z, 2 + std::sqrt(.5f)), "position changes by velocity times dt");
    world = World{};
    world.passByValue = true;
    updatePlayer(world, 1, 1, .5f);
    check(near(actualSpeed(world), std::sqrt(8.0f)), "copy experiment exposes faster diagonal");
    world = World{};
    world.autoWalk = true;
    updatePlayer(world, 0, 0, .5f);
    check(near(world.player.x, 1.6f) && near(world.player.z, 2.8f), "auto walk follows normalized target direction");
    for (int i = 0; i < 100; ++i) updatePlayer(world, 0, 0, .1f);
    check(near(distanceToFlag(world), 0) && !world.autoWalk && near(actualSpeed(world), 0), "auto walk stops at flag without overshoot");
    world = World{};
    world.player = {12, 0, 12};
    updatePlayer(world, 1, 1, 1);
    check(near(world.player.x, 12) && near(world.player.z, 12), "practice ground bounds are respected");
    check(near(actualSpeed(world), 0), "blocked movement reports zero actual speed");
    Vec3 forward{2, 0, 0}, up{1, 3, 0}, third{};
    const int result = third.makeOrthonormalBasis(forward, up);
    check(result == 0 && near(forward.x, 1) && near(up.y, 1) && near(third.z, 1), "original makeOrthonormalBasis builds orthogonal unit axes");
    Vec3 parallelA{1, 0, 0}, parallelB{2, 0, 0};
    check(third.makeOrthonormalBasis(parallelA, parallelB) == -20003, "original basis reports parallel directions");
    Vec3 point{3, 999, 4}, origin{};
    check(near(dice::hfe::distXZSqr(point, origin), 25), "original XZ distance ignores height");
    std::cout << count - failed << '/' << count << " checks passed\n";
    return failed ? 1 : 0;
}
