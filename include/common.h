#pragma once

#include <GL/freeglut.h>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------- Shared constants ----------
extern const float TANK_WIDTH;
extern const float TANK_HEIGHT;
extern const float TANK_DEPTH;
extern const float WATER_LEVEL;

// ---------- Shared state ----------
extern float globalTime;
extern bool isPaused;

// ---------- Shared data types ----------
struct Bubble { float x, y, z, speed, size, wobbleOffset; };
struct Plant  { float x, z, height, segmentLength, swayOffset; int segments; };

extern std::vector<Bubble> bubbles;
extern std::vector<Plant> plants;

// ---------- Small vector helpers ----------
struct V3 { float x, y, z; };

inline V3 v3sub(V3 a, V3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline V3 v3scale(V3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline float v3dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 v3cross(V3 a, V3 b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}
inline float clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }