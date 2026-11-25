#include "particles.h"
#include "common.h"

#include <cmath>
#include <cstdlib>
#include <vector>

namespace {
    struct Particle {
        float x, y, z;
        float speed;
        float phase;
        float pointSize;
        float alpha;
    };

    std::vector<Particle> particles;

    const int PARTICLE_COUNT = 100;
    const float EDGE_MARGIN = 0.6f;
    const float BOTTOM_LEVEL = 0.3f;

    float randomRange(float minimum, float maximum) {
        float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return minimum + (maximum - minimum) * t;
    }

    void resetParticle(Particle& p, bool randomHeight) {
        p.x = randomRange(-TANK_WIDTH / 2.0f + EDGE_MARGIN,
                           TANK_WIDTH / 2.0f - EDGE_MARGIN);
        p.z = randomRange(-TANK_DEPTH / 2.0f + EDGE_MARGIN,
                           TANK_DEPTH / 2.0f - EDGE_MARGIN);

        p.y = randomHeight
            ? randomRange(BOTTOM_LEVEL, WATER_LEVEL - 0.5f)
            : BOTTOM_LEVEL;

        // Much slower than the rising bubbles.
        p.speed = randomRange(0.06f, 0.18f);
        p.phase = randomRange(0.0f, 2.0f * static_cast<float>(M_PI));

        p.pointSize = randomRange(1.5f, 3.0f);
        p.alpha = randomRange(0.25f, 0.55f);
    }
}

void initParticles() {
    particles.clear();
    particles.reserve(PARTICLE_COUNT);

    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        Particle p;
        resetParticle(p, true);
        particles.push_back(p);
    }
}

void updateParticles(float dt) {
    for (Particle& p : particles) {
        p.y += p.speed * dt;

        // Recycle particles that reach the water surface.
        if (p.y >= WATER_LEVEL - 0.4f) {
            resetParticle(p, false);
        }
    }
}

void drawParticles() {
    // Save the OpenGL states changed by this function.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_POINT_BIT | GL_CURRENT_BIT);

    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POINT_SMOOTH);

    for (const Particle& p : particles) {
        float driftX = std::sin(globalTime * 0.45f + p.phase) * 0.18f;
        float driftZ = std::cos(globalTime * 0.35f + p.phase) * 0.18f;

        glPointSize(p.pointSize);
        glColor4f(0.75f, 0.90f, 1.0f, p.alpha);

        glBegin(GL_POINTS);
        glVertex3f(p.x + driftX, p.y, p.z + driftZ);
        glEnd();
    }

    // Restore lighting, blending, depth mask, color, and point settings.
    glPopAttrib();
}
