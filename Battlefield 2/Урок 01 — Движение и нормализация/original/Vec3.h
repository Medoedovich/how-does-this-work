// Vector types and arithmetic. [INFERRED] Source placement.

#ifndef MATH_VEC3_H     // [INFERRED] guard style
#define MATH_VEC3_H

#include <iosfwd>

namespace dice {
namespace hfe {
namespace Math {

struct Vec3
{
    float x, y, z;

    // Returns a reference to the shared zero vector.
    static const Vec3& getZero();
    static const Vec3& getIdentity();


    // Builds an orthonormal frame in place: this receives the third axis, and forward/up are rewritten.
    // Returns zero or -20003 for a degenerate vector. A later degenerate up vector is zeroed without failure.
    int makeOrthonormalBasis(Vec3& forward, Vec3& up);
};


// Parses three components separated by any single non-whitespace character; prints x/y/z.
// [CHOSEN] Stream operators initialize failed component reads to zero rather than retaining undefined stack data.

std::istream& operator>>(std::istream& in, Vec3& v);
std::ostream& operator<<(std::ostream& out, const Vec3& v);

// [CHOSEN] Stream operators for slash-separated Vec2 coordinates.
struct Vec2;
std::istream& operator>>(std::istream& in, Vec2& v);
std::ostream& operator<<(std::ostream& out, const Vec2& v);

// Parses or prints slash-separated Vec4 components. [CHOSEN] Header packaging.
struct Vec4;
std::istream& operator>>(std::istream& in, Vec4& v);
std::ostream& operator<<(std::ostream& out, const Vec4& v);

// Four-component float vector. [CHOSEN, not recovered] Field names and component representation.
struct Vec4
{
    float x, y, z, w;
};

struct Vec2
{
    float x, y;
};

// Colour plus intensity in one token group - four numbers, not three. Console metadata declares
// this type for `color`, `speccolor` and the sun colour properties.
struct Vec3ColorFloatIntensity
{
    float r, g, b;
    float intensity;
};

} // namespace Math

// The console API and several struct layouts use the unqualified spelling for the same types.
typedef Math::Vec3 Vec3;
typedef Math::Vec2 Vec2;
typedef Math::Vec4 Vec4;


// Projects between Vec3 and the XZ plane through output parameters. Y is discarded or restored.
// [INFERRED] Void return contracts; preserve the output-parameter signatures.

void vec3ToVec2XZ(const Vec3& in, Vec2& out);
void vec2ToVec3XZ(const Vec2& in, Vec3& out);
// Squared distance in the XZ plane; ignores y. [INFERRED] Float return.
// Native return rounds once; original x87 callers could compare the unrounded value.
float distXZSqr(const Vec3& a, const Vec3& b);

// Computes (centre - minimum) + centre, storing x before reading y.
// Output may alias either input. [INFERRED] Reference return.
Vec2& getBoxMaxCorner(const Vec2& minimum, const Vec2& centre, Vec2& output);

} // namespace hfe
} // namespace dice

#endif // MATH_VEC3_H
