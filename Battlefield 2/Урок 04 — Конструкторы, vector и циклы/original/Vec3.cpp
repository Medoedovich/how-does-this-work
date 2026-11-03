// Shared vectors, basis construction, coordinate helpers, and text serialization.

#include "Vec3.h"
#include <cmath>        // makeOrthonormalBasis
#include <istream>      // the console Vec3 text form below
#include <ostream>

namespace dice {
namespace hfe {
namespace Math {


// [CHOSEN, not recovered] Stable zero-vector storage for reference-returning accessors.

namespace {
const Vec3 s_zero = { 0.0f, 0.0f, 0.0f };
const Vec3 s_identity = { 1.0f, 1.0f, 1.0f };
}

const Vec3& Vec3::getZero()
{
    return s_zero;
}


// Basis construction uses FLT_EPSILON for each normalization.

namespace {

const float BASIS_EPS = 1.19209290e-7f;                  // FLT_EPSILON, as the body spells it

// The inlined "normalise unless it already is" step. False = degenerate; the caller zeroes.
bool normaliseIfNeeded(Vec3& v)
{
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (std::fabs(lenSq - 1.0f) < BASIS_EPS)
        return true;                                     // already unit - untouched
    if (std::fabs(lenSq) < BASIS_EPS)
        return false;
    const float inv = 1.0f / std::sqrt(lenSq);
    v.x *= inv; v.y *= inv; v.z *= inv;
    return true;
}

void zero(Vec3& v) { v.x = 0.0f; v.y = 0.0f; v.z = 0.0f; }

void cross(Vec3& out, const Vec3& a, const Vec3& b)
{
    out.x = a.y * b.z - a.z * b.y;
    out.y = a.z * b.x - a.x * b.z;
    out.z = a.x * b.y - a.y * b.x;
}

void snapSmall(Vec3& v)
{
    if (std::fabs(v.x) < BASIS_EPS) v.x = 0.0f;
    if (std::fabs(v.y) < BASIS_EPS) v.y = 0.0f;
    if (std::fabs(v.z) < BASIS_EPS) v.z = 0.0f;
}

} // namespace

int Vec3::makeOrthonormalBasis(Vec3& forward, Vec3& up)
{
    if (!normaliseIfNeeded(forward)) { zero(forward); return -20003; }
    if (!normaliseIfNeeded(up))      { zero(up);      return -20003; }

    cross(*this, forward, up);
    if (!normaliseIfNeeded(*this))   { zero(*this);   return -20003; }

    // The one step that does not abandon the basis on failure - see the header.
    cross(up, *this, forward);
    if (!normaliseIfNeeded(up))
        zero(up);

    cross(forward, up, *this);
    if (!normaliseIfNeeded(forward)) { zero(forward); return -20003; }

    snapSmall(*this);
    snapSmall(forward);
    snapSmall(up);
    return 0;
}

const Vec3& Vec3::getIdentity()
{
    return s_identity;
}

// [CHOSEN] Stream operators for slash-separated vector coordinates.
std::istream& operator>>(std::istream& in, Vec3& v)
{
    float f = 0.0f;
    char sep = 0;
    in >> f;                // istream::operator>>(float&)
    in >> sep;              // one plain char - the '/' (any single char accepted)
    v.x = f;
    in >> f;
    in >> sep;
    v.y = f;
    in >> f;                // no separator after the last component
    v.z = f;
    return in;
}

std::ostream& operator<<(std::ostream& out, const Vec3& v)
{
    out << v.x << '/' << v.y << '/' << v.z;    // default precision, '/' separators
    return out;
}

// The Vec2 form: float >> char >> float / x '/' y (the hoistMinMax pair, ConsoleClass1259).
std::istream& operator>>(std::istream& in, Vec2& v)
{
    float f = 0.0f;
    char sep = 0;
    in >> f;
    in >> sep;
    v.x = f;
    in >> f;
    v.y = f;
    return in;
}

std::ostream& operator<<(std::ostream& out, const Vec2& v)
{
    out << v.x << '/' << v.y;
    return out;
}

// Parses/prints four slash-separated components; separators are consumed without validation.
std::istream& operator>>(std::istream& in, Vec4& v)
{
    float f = 0.0f;
    char sep = 0;
    in >> f;
    in >> sep;
    v.x = f;
    in >> f;
    in >> sep;
    v.y = f;
    in >> f;
    in >> sep;
    v.z = f;
    in >> f;                // no separator after the last component
    v.w = f;
    return in;
}

std::ostream& operator<<(std::ostream& out, const Vec4& v)
{
    out << v.x << '/' << v.y << '/' << v.z << '/' << v.w;
    return out;
}

}
// Projects x/z into Vec2, discarding height.

void vec3ToVec2XZ(const Vec3& in, Vec2& out)
{
    out.x = in.x;
    out.y = in.z;
}


// Maps Vec2 x/y to Vec3 x/z and explicitly clears height.

void vec2ToVec3XZ(const Vec2& in, Vec3& out)
{
    out.x = in.x;
    out.y = 0.0f;
    out.z = in.y;
}

// Squared XZ distance; keep differences and products in double precision.
float distXZSqr(const Vec3& a, const Vec3& b)
{
    const double dx = static_cast<double>(a.x) - b.x;
    const double dz = static_cast<double>(a.z) - b.z;
    return static_cast<float>(dx * dx + dz * dz);
}

// Computes (centre - minimum) + centre in double precision before each float store.
// Stores x before reading y; output may alias either input. Returns output.
Vec2& getBoxMaxCorner(const Vec2& minimum, const Vec2& centre, Vec2& output)
{
    output.x = static_cast<float>((static_cast<double>(centre.x) - minimum.x) + centre.x);
    output.y = static_cast<float>((static_cast<double>(centre.y) - minimum.y) + centre.y);
    return output;
}

} // namespace hfe
} // namespace dice
