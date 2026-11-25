#include "environment.h"
#include <cmath>
#include <cstdlib>

static float randomX() { return ((rand() % 100) / 100.0f * TANK_WIDTH) - TANK_WIDTH / 2; }
static float randomZ() { return ((rand() % 100) / 100.0f * TANK_DEPTH) - TANK_DEPTH / 2; }

void initEnvironment() {
    for (int i = 0; i < 50; i++) {
        Bubble b;
        b.x = randomX();
        b.y = ((rand() % 100) / 100.0f * WATER_LEVEL);
        b.z = randomZ();
        b.speed = 1.0f + (rand() % 20) / 10.0f;
        b.size = 0.05f + (rand() % 10) / 100.0f;
        b.wobbleOffset = (rand() % 314) / 100.0f;
        bubbles.push_back(b);
    }

    for (int i = 0; i < 20; i++) {
        Plant p;
        p.x = randomX();
        p.z = randomZ();
        p.height = 3.0f + (rand() % 40) / 10.0f;
        p.segments = 8;
        p.segmentLength = p.height / p.segments;
        p.swayOffset = (rand() % 314) / 100.0f;
        plants.push_back(p);
    }
}

void updateBubbles(float dt) {
    for (auto& b : bubbles) {
        b.y += b.speed * dt;
        // Reset when the bubble reaches the water surface
        if (b.y > WATER_LEVEL - 0.3f) {
            b.y = 0.0f;
            b.x = randomX();
            b.z = randomZ();
        }
    }
}

void drawSandBottom() {
    GLfloat sandColor[] = { 0.7f, 0.7f, 0.5f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sandColor);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-TANK_WIDTH/2, 0, TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, 0, TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, 0, -TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, 0, -TANK_DEPTH/2);
    glEnd();
}

void drawRocks() {
    const GLfloat rockColors[][4] = {
        {0.32f, 0.36f, 0.36f, 1.0f},
        {0.43f, 0.39f, 0.32f, 1.0f},
        {0.27f, 0.32f, 0.34f, 1.0f}
    };
    struct Rock { float x, z, scale; int color; };
    const Rock rocks[] = {
        {-8.0f, -5.3f, 0.62f, 0}, {-7.4f, -4.8f, 0.40f, 1},
        {-8.1f,  5.3f, 0.52f, 2}, {-7.5f,  5.6f, 0.34f, 1},
        { 8.0f, -5.4f, 0.58f, 1}, { 7.4f, -4.9f, 0.38f, 0},
        { 8.0f,  5.2f, 0.54f, 2}, { 7.4f,  5.6f, 0.35f, 0},
        {-2.6f, -6.5f, 0.35f, 2}, { 2.2f, -6.4f, 0.42f, 1}
    };

    for (const Rock& rock : rocks) {
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, rockColors[rock.color]);
        glPushMatrix();
        glTranslatef(rock.x, rock.scale * 0.78f, rock.z);
        glScalef(rock.scale * 1.25f, rock.scale * 0.78f, rock.scale);
        glutSolidSphere(1.0, 16, 12);
        glPopMatrix();
    }
}

void drawPlants() {
    GLfloat plantColor[] = { 0.1f, 0.6f, 0.2f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, plantColor);
    GLfloat noSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, noSpecular);

    for (const auto& p : plants) {
        glPushMatrix();
        glTranslatef(p.x, 0.0f, p.z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

        for (int i = 0; i < p.segments; i++) {
            float sway = std::sin(globalTime + p.swayOffset + i * 0.5f) * 5.0f;
            glRotatef(sway, 0.0f, 1.0f, 0.0f);
            glutSolidCylinder(0.15f - (i * 0.015f), p.segmentLength, 8, 1);
            glTranslatef(0.0f, 0.0f, p.segmentLength);
        }
        glPopMatrix();
    }
}

void drawBubbles() {
    // Sets the state it needs by itself, so it does not depend on other functions.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat bubbleColor[] = { 0.9f, 0.95f, 1.0f, 0.4f };
    GLfloat bubbleSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, bubbleColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, bubbleSpec);
    glMaterialf(GL_FRONT, GL_SHININESS, 120.0f);

    for (const auto& b : bubbles) {
        glPushMatrix();
        float wobbleX = std::sin(globalTime * 3.0f + b.wobbleOffset) * 0.2f;
        float wobbleZ = std::cos(globalTime * 3.0f + b.wobbleOffset) * 0.2f;

        glTranslatef(b.x + wobbleX, b.y, b.z + wobbleZ);
        glutSolidSphere(b.size, 10, 10);
        glPopMatrix();
    }
}

// ---------- Light rays ----------

// Draw one soft ribbon of light.
// horizontalX = true: ribbon spreads along X.
// horizontalX = false: ribbon spreads along Z.
static void drawLightRibbon(float centerX, float centerZ, float phase, bool horizontalX) {
    const int verticalSteps = 12;
    const int widthSteps = 8;

    const float topY = WATER_LEVEL - 0.35f;
    const float bottomY = 0.45f;

    float brightness = 0.85f + 0.15f * std::sin(globalTime * 0.6f + phase);

    for (int row = 0; row < verticalSteps; ++row) {
        glBegin(GL_QUAD_STRIP);

        for (int col = 0; col <= widthSteps; ++col) {
            // Across the ribbon: -1 at one edge, +1 at the other.
            float across = -1.0f + 2.0f * col / static_cast<float>(widthSteps);

            // Zero brightness at both side edges.
            float sideFade = 1.0f - across * across;

            for (int edge = 0; edge < 2; ++edge) {
                // t = 0 at the top; t = 1 at the bottom.
                float t = (row + edge) / static_cast<float>(verticalSteps);

                float y = topY + (bottomY - topY) * t;

                // Beam becomes wider toward the bottom.
                float halfWidth = 0.18f + 0.65f * t;

                // Gentle slant and animated sideways movement.
                float sway = std::sin(globalTime * 0.4f + phase + t) * 0.18f;

                float x = centerX + 0.7f * t + sway;
                float z = centerZ + 0.25f * t;

                if (horizontalX) {
                    x += across * halfWidth;
                } else {
                    z += across * halfWidth;
                }

                // Fade in near the top and fade out near the bottom.
                float topFade = clamp01(t / 0.12f);
                float bottomFade = 1.0f - t;

                float alpha = 0.10f * brightness * sideFade * topFade * bottomFade;

                glColor4f(0.65f, 0.85f, 1.0f, alpha);
                glVertex3f(x, y, z);
            }
        }

        glEnd();
    }
}

void drawLightRays() {
    // Preserve the states changed by this effect.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_CURRENT_BIT | GL_LIGHTING_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);        // fog would tint the additive colour
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    // Opaque objects still hide the beam, but the beam does not write depth.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Additive blending: the beam adds light to what is behind it.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Beam centres (x, z)
    const float positions[3][2] = {
        {-6.0f, -3.5f},
        {-1.5f,  1.0f},
        { 4.0f, -2.5f}
    };

    for (int i = 0; i < 3; ++i) {
        float phase = i * 1.7f;

        // Two crossed ribbons so the beam is visible from more angles.
        drawLightRibbon(positions[i][0], positions[i][1], phase, true);
        drawLightRibbon(positions[i][0], positions[i][1], phase, false);
    }

    glPopAttrib();
}