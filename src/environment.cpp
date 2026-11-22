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