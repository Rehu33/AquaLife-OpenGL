#include "water.h"
#include <cmath>

// Two overlapping sine waves in different directions give a more natural ripple.
// Wave 1: broad, slow swell.  Wave 2: smaller, faster ripples.
static float waveY(float x, float z, float t) {
    float w1 = std::sin(t * 2.0f + x * 0.5f + z * 0.4f) * 0.20f;
    float w2 = std::sin(t * 3.5f - x * 1.2f + z * 0.9f) * 0.07f;
    return WATER_LEVEL + w1 + w2;
}

// Normal = (-dy/dx, 1, -dy/dz), then normalized.
static V3 waveNormal(float x, float z, float t) {
    float c1 = std::cos(t * 2.0f + x * 0.5f + z * 0.4f) * 0.20f;
    float c2 = std::cos(t * 3.5f - x * 1.2f + z * 0.9f) * 0.07f;

    float dydx = c1 * 0.5f + c2 * (-1.2f);
    float dydz = c1 * 0.4f + c2 * 0.9f;

    V3 n = { -dydx, 1.0f, -dydz };
    float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    return { n.x / len, n.y / len, n.z / len };
}

void drawWaterSurface() {
    const int gridX = 40;
    const int gridZ = 30;

    GLfloat waterColor[] = { 0.45f, 0.80f, 1.0f, 0.70f };
    GLfloat waterSpec[]  = { 1.0f, 1.0f, 1.0f, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, waterColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, waterSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
    for (int i = 0; i < gridX; i++) {
        for (int j = 0; j < gridZ; j++) {
            float x1 = -TANK_WIDTH / 2 + (TANK_WIDTH * i) / gridX;
            float x2 = -TANK_WIDTH / 2 + (TANK_WIDTH * (i + 1)) / gridX;
            float z1 = -TANK_DEPTH / 2 + (TANK_DEPTH * j) / gridZ;
            float z2 = -TANK_DEPTH / 2 + (TANK_DEPTH * (j + 1)) / gridZ;

            V3 n;
            n = waveNormal(x1, z2, globalTime); glNormal3f(n.x, n.y, n.z);
            glVertex3f(x1, waveY(x1, z2, globalTime), z2);

            n = waveNormal(x2, z2, globalTime); glNormal3f(n.x, n.y, n.z);
            glVertex3f(x2, waveY(x2, z2, globalTime), z2);

            n = waveNormal(x2, z1, globalTime); glNormal3f(n.x, n.y, n.z);
            glVertex3f(x2, waveY(x2, z1, globalTime), z1);

            n = waveNormal(x1, z1, globalTime); glNormal3f(n.x, n.y, n.z);
            glVertex3f(x1, waveY(x1, z1, globalTime), z1);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_FOG);
}