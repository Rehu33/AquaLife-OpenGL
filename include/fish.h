#pragma once

#include "common.h"

// ---------- Fish body geometry helpers ----------
float profileR(float u);
float lateralOffset(float u, float phase);
V3 bodyPoint(float u, float v, float phase);
V3 bodyNormal(float u, float v, float phase);

class Fish {
public:
    float x, y, z;
    float heading, targetHeading;
    float speed;
    float cruiseY, verticalPhase, turnTimer;
    float tailAngle, finAngle;
    float wavePhase;
    float sizeScale;
    float r, g, b;

    Fish(float startX, float startY, float startZ, float scale, float red, float green, float blue);

    void update(float dt);
    void draw();

private:
    void applyColorStripe(float u);
    void drawBody(float phase);
    void drawDorsalFin(float phase);
    void drawCurvedFin(float scaleX, float scaleY, float scaleZ);
    void drawBeautifulTail();
    void setFinMaterial(float fr, float fg, float fb);
    void drawEye(float ex, float ey, float ez, float side);
};