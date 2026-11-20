#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float cameraAngleX = 20.0f;
float cameraAngleY = 45.0f;
float cameraDistance = 22.0f;

int lastTime = 0;
int lastMouseX = 0;
int lastMouseY = 0;
float globalTime = 0.0f;
bool isPaused = false;
bool isDraggingCamera = false;

const float TANK_WIDTH = 20.0f;
const float TANK_HEIGHT = 10.0f;
const float TANK_DEPTH = 15.0f;

// Water level (surface sits slightly below the tank rim)
const float WATER_LEVEL = TANK_HEIGHT - 0.35f;

struct Bubble { float x, y, z, speed, size, wobbleOffset; };
struct Plant { float x, z, height, segmentLength, swayOffset; int segments; };

std::vector<Bubble> bubbles;
std::vector<Plant> plants;

struct V3 { float x, y, z; };

static V3 v3sub(V3 a, V3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static V3 v3scale(V3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
static float v3dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static V3 v3cross(V3 a, V3 b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}
static float clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }

float profileR(float u) {
    u = clamp01(u);
    float f = std::pow(u, 0.6f);
    float s = std::sin((float)M_PI * f);
    if (s < 0.0f) s = 0.0f;
    return 0.10f + 0.85f * std::pow(s, 0.8f);
}

float lateralOffset(float u, float phase) {
    return (0.04f + 0.30f * u * u) * std::sin(4.5f * u - phase);
}

V3 bodyPoint(float u, float v, float phase) {
    float R = profileR(u);
    float sv = std::sin(v);
    float cv = std::cos(v);

    float x = 1.5f - 3.0f * u;
    float yScale = 1.15f + 0.15f * sv;
    float y = R * yScale * sv;
    float z = R * 0.45f * cv + lateralOffset(u, phase);
    return { x, y, z };
}

V3 bodyNormal(float u, float v, float phase) {
    const float e = 0.001f;
    float ua = clamp01(u - e), ub = clamp01(u + e);
    float va = v - e, vb = v + e;

    V3 Pu = v3scale(v3sub(bodyPoint(ub, v, phase), bodyPoint(ua, v, phase)), 1.0f / (ub - ua));
    V3 Pv = v3scale(v3sub(bodyPoint(u, vb, phase), bodyPoint(u, va, phase)), 1.0f / (vb - va));
    V3 n = v3cross(Pu, Pv);

    V3 P = bodyPoint(u, v, phase);
    float off = lateralOffset(u, phase);
    V3 outward = { 0.0f, P.y, P.z - off };
    if (v3dot(n, outward) < 0.0f) n = v3scale(n, -1.0f);

    float len = std::sqrt(v3dot(n, n));
    if (len < 1e-6f) return { 0.0f, 1.0f, 0.0f };
    return v3scale(n, 1.0f / len);
}

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

    Fish(float startX, float startY, float startZ, float scale, float red, float green, float blue) {
        x = startX; y = startY; z = startZ;
        heading = rand() % 360;
        targetHeading = heading;
        speed = 0.8f + (rand() % 10) / 10.0f;
        cruiseY = startY;
        verticalPhase = (rand() % 628) / 100.0f;
        turnTimer = 2.0f + (rand() % 300) / 100.0f;
        sizeScale = scale;
        tailAngle = 0.0f; finAngle = 0.0f; wavePhase = 0.0f;
        r = red; g = green; b = blue;
    }

    void update(float dt) {
        wavePhase = globalTime * speed * 6.0f;
        tailAngle = std::sin(wavePhase) * 18.0f;
        finAngle  = std::sin(globalTime * speed * 3.0f) * 20.0f;
        y = cruiseY + std::sin(globalTime * speed * 0.7f + verticalPhase) * 0.38f;
        y = std::max(1.0f, std::min(y, WATER_LEVEL - 1.5f));

        float margin = 3.0f;
        float limitX = (TANK_WIDTH / 2.0f) - margin;
        float limitZ = (TANK_DEPTH / 2.0f) - margin;

        float forceX = 0.0f, forceZ = 0.0f;
        if (x > limitX) forceX = limitX - x;
        if (x < -limitX) forceX = -limitX - x;
        if (z > limitZ) forceZ = limitZ - z;
        if (z < -limitZ) forceZ = -limitZ - z;

        if (forceX != 0.0f || forceZ != 0.0f) {
            float escapeHeading = std::atan2(-forceZ, forceX) * (180.0f / (float)M_PI);
            targetHeading = escapeHeading;
            turnTimer = 2.0f;
            float diff = targetHeading - heading;
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            heading += diff * std::min(4.0f * dt, 1.0f);
        } else {
            turnTimer -= dt;
            if (turnTimer <= 0.0f) {
                targetHeading += (rand() % 81) - 40;
                turnTimer = 2.0f + (rand() % 300) / 100.0f;
            }
            float diff = targetHeading - heading;
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            heading += diff * std::min(1.5f * dt, 1.0f);
        }

        float radHeading = heading * ((float)M_PI / 180.0f);
        x += std::cos(radHeading) * speed * dt;
        z -= std::sin(radHeading) * speed * dt;

        if (x > TANK_WIDTH/2 - 0.5f) x = TANK_WIDTH/2 - 0.5f;
        if (x < -TANK_WIDTH/2 + 0.5f) x = -TANK_WIDTH/2 + 0.5f;
        if (z > TANK_DEPTH/2 - 0.5f) z = TANK_DEPTH/2 - 0.5f;
        if (z < -TANK_DEPTH/2 + 0.5f) z = -TANK_DEPTH/2 + 0.5f;
    }

    void applyColorStripe(float u) {
        if ((u > 0.2f && u < 0.3f) || (u > 0.6f && u < 0.7f)) {
            GLfloat white[] = { 0.95f, 0.95f, 0.95f, 1.0f };
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
        } else {
            GLfloat baseCol[] = { r, g, b, 1.0f };
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, baseCol);
        }
    }

    void drawBody(float phase) {
        const int slices = 60;
        const int stacks = 26;

        for (int i = 0; i < slices; i++) {
            float u1 = (float)i / slices;
            float u2 = (float)(i + 1) / slices;
            float uMid = (u1 + u2) * 0.5f;

            applyColorStripe(uMid);

            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= stacks; j++) {
                float v = (float)j / stacks * 2.0f * (float)M_PI;

                V3 n1 = bodyNormal(u1, v, phase);
                V3 p1 = bodyPoint(u1, v, phase);
                glNormal3f(n1.x, n1.y, n1.z);
                glVertex3f(p1.x, p1.y, p1.z);

                V3 n2 = bodyNormal(u2, v, phase);
                V3 p2 = bodyPoint(u2, v, phase);
                glNormal3f(n2.x, n2.y, n2.z);
                glVertex3f(p2.x, p2.y, p2.z);
            }
            glEnd();
        }
    }

    void drawDorsalFin(float phase) {
        const int steps = 18;
        const float uStart = 0.14f, uEnd = 0.86f;

        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.0f, 0.0f, 1.0f);
        for (int i = 0; i <= steps; i++) {
            float t = (float)i / steps;
            float u = uStart + (uEnd - uStart) * t;
            V3 base = bodyPoint(u, (float)M_PI * 0.5f, phase);
            float h = 0.55f * std::sin((float)M_PI * t);
            glVertex3f(base.x, base.y - 0.02f, base.z);
            glVertex3f(base.x - 0.25f * t, base.y + h, base.z);
        }
        glEnd();
    }

    void drawCurvedFin(float scaleX, float scaleY, float scaleZ) {
        glPushMatrix();
        glScalef(scaleX, scaleY, scaleZ);

        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);

        for (int i = 0; i <= 10; i++) {
            float t = i / 10.0f;
            float px = -1.2f * t - 0.3f * std::sin(t * (float)M_PI);
            float py = 1.2f * std::sin(t * (float)M_PI * 0.7f);
            glVertex3f(px, py, 0.0f);
        }
        glEnd();

        glPopMatrix();
    }

    void drawBeautifulTail() {
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);

        for (int i = 0; i <= 15; i++) {
            float angle = -1.0f + (2.0f * i / 15.0f);
            float length = 1.5f + 0.4f * std::cos(angle * 3.0f);
            glVertex3f(-length * std::cos(angle), length * std::sin(angle), 0.0f);
        }
        glEnd();
    }

    void setFinMaterial(float fr, float fg, float fb) {
        GLfloat c[] = { fr, fg, fb, 0.8f };
        GLfloat spec[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, c);
        glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    }

    void drawEye(float ex, float ey, float ez, float side) {
        GLfloat eyeWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat eyeBlack[] = { 0.05f, 0.05f, 0.05f, 1.0f };
        GLfloat eyeHighlight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat noSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f };

        glMaterialfv(GL_FRONT, GL_SPECULAR, noSpecular);

        glPushMatrix();
        glTranslatef(ex, ey, ez);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, eyeWhite);
        glutSolidSphere(0.16, 16, 16);

        glTranslatef(0.08f, 0.0f, 0.1f * side);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, eyeBlack);
        glutSolidSphere(0.09, 16, 16);

        glTranslatef(0.05f, 0.03f, 0.04f * side);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, eyeHighlight);
        glutSolidSphere(0.025, 8, 8);
        glPopMatrix();
    }

    void draw() {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(heading, 0.0f, 1.0f, 0.0f);
        glScalef(sizeScale, sizeScale, sizeScale);

        GLfloat matAmbient[] = { r * 0.4f, g * 0.4f, b * 0.4f, 1.0f };
        GLfloat matDiffuse[] = { r, g, b, 1.0f };
        GLfloat matSpecular[] = { 0.6f, 0.6f, 0.6f, 1.0f };
        GLfloat matShininess[] = { 50.0f };

        glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

        drawBody(wavePhase);

        float dHead = lateralOffset(0.13f, wavePhase);
        drawEye(1.1f, 0.3f,  0.3f + dHead,  1.0f);
        drawEye(1.1f, 0.3f, -0.3f + dHead, -1.0f);

        setFinMaterial(r * 0.85f, g * 0.85f, b * 0.85f);
        drawDorsalFin(wavePhase);

        float dPec = lateralOffset(0.33f, wavePhase);
        setFinMaterial(r * 0.8f, g * 0.8f, b * 0.8f);

        glPushMatrix();
        glTranslatef(0.5f, -0.3f, 0.4f + dPec);
        glRotatef(finAngle, 1.0f, 0.0f, 0.0f);
        glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);
        drawCurvedFin(0.7f, 0.5f, 0.1f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.5f, -0.3f, -0.4f + dPec);
        glRotatef(-finAngle, 1.0f, 0.0f, 0.0f);
        glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        drawCurvedFin(0.7f, 0.5f, 0.1f);
        glPopMatrix();

        float dTail = lateralOffset(1.0f, wavePhase);
        glPushMatrix();
        glTranslatef(-1.45f, 0.0f, dTail);
        glRotatef(tailAngle, 0.0f, 1.0f, 0.0f);
        setFinMaterial(r * 0.8f, g * 0.8f, b * 0.8f);
        drawBeautifulTail();

        glDisable(GL_LIGHTING);
        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_LINES);
        for (int i = 1; i < 14; i++) {
            float angle = -1.0f + (2.0f * i / 15.0f);
            float length = 1.4f + 0.4f * std::cos(angle * 3.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(-length * std::cos(angle) * 0.9f, length * std::sin(angle) * 0.9f, 0.0f);
        }
        glEnd();
        glEnable(GL_LIGHTING);
        glPopMatrix();

        glPopMatrix();
    }
};

std::vector<Fish> schoolOfFish;

void initEnvironment() {
    for (int i = 0; i < 50; i++) {
        Bubble b;
        b.x = ((rand() % 100) / 100.0f * TANK_WIDTH) - TANK_WIDTH / 2;
        b.y = ((rand() % 100) / 100.0f * WATER_LEVEL);
        b.z = ((rand() % 100) / 100.0f * TANK_DEPTH) - TANK_DEPTH / 2;
        b.speed = 1.0f + (rand() % 20) / 10.0f;
        b.size = 0.05f + (rand() % 10) / 100.0f;
        b.wobbleOffset = (rand() % 314) / 100.0f;
        bubbles.push_back(b);
    }

    for (int i = 0; i < 20; i++) {
        Plant p;
        p.x = ((rand() % 100) / 100.0f * TANK_WIDTH) - TANK_WIDTH / 2;
        p.z = ((rand() % 100) / 100.0f * TANK_DEPTH) - TANK_DEPTH / 2;
        p.height = 3.0f + (rand() % 40) / 10.0f;
        p.segments = 8;
        p.segmentLength = p.height / p.segments;
        p.swayOffset = (rand() % 314) / 100.0f;
        plants.push_back(p);
    }

    schoolOfFish.push_back(Fish(0, 4, 0,  0.8f, 1.0f, 0.4f, 0.0f));
    schoolOfFish.push_back(Fish(3, 6, 2,  0.6f, 0.1f, 0.5f, 1.0f));
    schoolOfFish.push_back(Fish(-4, 3, -3, 0.9f, 1.0f, 0.1f, 0.3f));
    schoolOfFish.push_back(Fish(2, 7, -5, 0.7f, 1.0f, 0.9f, 0.0f));
    schoolOfFish.push_back(Fish(-5, 5, 4,  0.7f, 0.8f, 0.2f, 0.8f));
}

void initGL() {
    glClearColor(0.05f, 0.15f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightAmbient[] = { 0.3f, 0.4f, 0.5f, 1.0f };
    GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    glEnable(GL_FOG);
    GLfloat fogColor[] = {0.05f, 0.15f, 0.3f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.025f);
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
    // This function sets the state it needs by itself,
    // so it does not depend on what other functions did before.
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

// ---------- Water surface ----------

// Two overlapping sine waves in different directions give a more natural ripple.
// Wave 1: broad, slow swell.  Wave 2: smaller, faster ripples.
static float waveY(float x, float z, float t) {
    float w1 = std::sin(t * 2.0f + x * 0.5f + z * 0.4f) * 0.20f;
    float w2 = std::sin(t * 3.5f - x * 1.2f + z * 0.9f) * 0.07f;
    return WATER_LEVEL + w1 + w2;
}

// Normal from the partial derivatives of waveY.
// For y = A*sin(k·p + wt), dy/dx = A*kx*cos(...), dy/dz = A*kz*cos(...)
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

    // FRONT_AND_BACK so the water has colour from above and below
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

            // Counter-clockwise when viewed from above (+Y)
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
    // Note: blending is NOT disabled here, so later transparent objects still work.
}

void drawGlassTank() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.3f, 0.6f, 0.9f, 0.15f);

    glBegin(GL_QUADS);
    glVertex3f(-TANK_WIDTH/2, 0, -TANK_DEPTH/2); glVertex3f(TANK_WIDTH/2, 0, -TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2); glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2);

    glVertex3f(-TANK_WIDTH/2, 0, TANK_DEPTH/2); glVertex3f(-TANK_WIDTH/2, 0, -TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2); glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);

    glVertex3f(TANK_WIDTH/2, 0, -TANK_DEPTH/2); glVertex3f(TANK_WIDTH/2, 0, TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2); glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2);

    glVertex3f(TANK_WIDTH/2, 0, TANK_DEPTH/2); glVertex3f(-TANK_WIDTH/2, 0, TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2); glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);
    glEnd();

    glColor4f(0.5f, 0.8f, 1.0f, 0.8f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex3f(-TANK_WIDTH/2, 0, -TANK_DEPTH/2); glVertex3f(TANK_WIDTH/2, 0, -TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, 0, -TANK_DEPTH/2);  glVertex3f(TANK_WIDTH/2, 0, TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, 0, TANK_DEPTH/2);   glVertex3f(-TANK_WIDTH/2, 0, TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, 0, TANK_DEPTH/2);  glVertex3f(-TANK_WIDTH/2, 0, -TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2); glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2);  glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);   glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);  glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, 0, -TANK_DEPTH/2); glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, 0, -TANK_DEPTH/2);  glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, -TANK_DEPTH/2);
    glVertex3f(TANK_WIDTH/2, 0, TANK_DEPTH/2);   glVertex3f(TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);
    glVertex3f(-TANK_WIDTH/2, 0, TANK_DEPTH/2);  glVertex3f(-TANK_WIDTH/2, TANK_HEIGHT, TANK_DEPTH/2);
    glEnd();

    glEnable(GL_LIGHTING);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float camRadX = cameraAngleX * (float)M_PI / 180.0f;
    float camRadY = cameraAngleY * (float)M_PI / 180.0f;

    float camX = cameraDistance * std::cos(camRadX) * std::sin(camRadY);
    float camY = cameraDistance * std::sin(camRadX) + (TANK_HEIGHT / 2.0f);
    float camZ = cameraDistance * std::cos(camRadX) * std::cos(camRadY);

    gluLookAt(camX, camY, camZ, 0.0, TANK_HEIGHT / 2.0f, 0.0, 0.0, 1.0, 0.0);

    GLfloat lightPos[] = { 0.0f, TANK_HEIGHT + 10.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // 1) Opaque objects first
    glDepthMask(GL_TRUE);
    drawSandBottom();
    drawRocks();
    drawPlants();
    for (auto& fish : schoolOfFish) fish.draw();

    // 2) Transparent water surface
    drawWaterSurface();

    // 3) Other transparent objects: do not write depth
    glDepthMask(GL_FALSE);
    drawBubbles();
    drawGlassTank();
    glDepthMask(GL_TRUE);

    glutSwapBuffers();
}

void update(int) {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = std::min((currentTime - lastTime) / 1000.0f, 0.05f);
    lastTime = currentTime;

    if (!isPaused) {
        globalTime += deltaTime;
        for (auto& fish : schoolOfFish) fish.update(deltaTime);
        for (auto& b : bubbles) {
            b.y += b.speed * deltaTime;
            // Reset when the bubble reaches the water surface
            if (b.y > WATER_LEVEL - 0.3f) {
                b.y = 0.0f;
                b.x = ((rand() % 100) / 100.0f * TANK_WIDTH) - TANK_WIDTH / 2;
                b.z = ((rand() % 100) / 100.0f * TANK_DEPTH) - TANK_DEPTH / 2;
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 27: exit(0); break;
        case ' ': isPaused = !isPaused; break;
        case 'r': case 'R': cameraAngleX = 20.0f; cameraAngleY = 45.0f; cameraDistance = 22.0f; break;
        case '=': case '+': cameraDistance -= 1.0f; if (cameraDistance < 5.0f) cameraDistance = 5.0f; break;
        case '-': case '_': cameraDistance += 1.0f; if (cameraDistance > 50.0f) cameraDistance = 50.0f; break;
    }
    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        isDraggingCamera = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    } else if (button == 3 && state == GLUT_DOWN) {
        cameraDistance = std::max(cameraDistance - 1.0f, 5.0f);
    } else if (button == 4 && state == GLUT_DOWN) {
        cameraDistance = std::min(cameraDistance + 1.0f, 50.0f);
    }
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    if (!isDraggingCamera) return;

    cameraAngleY += (x - lastMouseX) * 0.35f;
    cameraAngleX = std::max(-10.0f, std::min(cameraAngleX + (y - lastMouseY) * 0.35f, 85.0f));
    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

void specialKeys(int key, int, int) {
    switch (key) {
        case GLUT_KEY_UP: cameraAngleX += 2.0f; if (cameraAngleX > 85.0f) cameraAngleX = 85.0f; break;
        case GLUT_KEY_DOWN: cameraAngleX -= 2.0f; if (cameraAngleX < -10.0f) cameraAngleX = -10.0f; break;
        case GLUT_KEY_LEFT: cameraAngleY -= 2.0f; break;
        case GLUT_KEY_RIGHT: cameraAngleY += 2.0f; break;
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    srand(static_cast<unsigned>(time(0)));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(1024, 768);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("AquaLife: Cute & Beautiful 3D Aquarium");

    initGL();
    initEnvironment();
    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}