#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
 
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
 
// ---------------------------------------------------------
// Global State & Camera Variables
// ---------------------------------------------------------
float cameraAngleX = 20.0f;
float cameraAngleY = 45.0f;
float cameraDistance = 22.0f;
 
int lastTime = 0;
float globalTime = 0.0f;
bool isPaused = false;
 
const float TANK_WIDTH = 20.0f;
const float TANK_HEIGHT = 10.0f;
const float TANK_DEPTH = 15.0f;
 
struct Bubble { float x, y, z, speed, size, wobbleOffset; };
struct Plant { float x, z, height, segmentLength, swayOffset; int segments; };
 
std::vector<Bubble> bubbles;
std::vector<Plant> plants;
 
// ---------------------------------------------------------
// Small vector helpers (used for fish surface maths)
// ---------------------------------------------------------
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
 
// ---------------------------------------------------------
// Fish body geometry
//   u : 0 (nose) -> 1 (tail tip)
//   v : 0 -> 2*pi around the body
//   phase : current swimming-wave phase
// The body is a bent, flattened, fish-shaped tube.
// ---------------------------------------------------------
 
// Body radius along the length: full at about 1/3 from the nose,
// tapering to a slim tail stalk (peduncle) at the back.
float profileR(float u) {
    u = clamp01(u);
    float f = std::pow(u, 0.6f);
    float s = std::sin((float)M_PI * f);
    if (s < 0.0f) s = 0.0f;
    return 0.10f + 0.85f * std::pow(s, 0.8f);
}
 
// Side-to-side bend of the body centreline (travelling wave, head -> tail).
// Amplitude grows toward the tail, like a real fish.
float lateralOffset(float u, float phase) {
    return (0.04f + 0.30f * u * u) * std::sin(4.5f * u - phase);
}
 
// Point on the body surface.
V3 bodyPoint(float u, float v, float phase) {
    float R = profileR(u);
    float sv = std::sin(v);
    float cv = std::cos(v);
 
    float x = 1.5f - 3.0f * u;
    // Back (top) is slightly taller than the belly, and the blend is smooth (no crease).
    float yScale = 1.15f + 0.15f * sv;
    float y = R * yScale * sv;
    float z = R * 0.45f * cv + lateralOffset(u, phase);
    return { x, y, z };
}
 
// Outward-facing normal, computed from the surface itself using
// finite differences (cross product of the two tangents).
V3 bodyNormal(float u, float v, float phase) {
    const float e = 0.001f;
    float ua = clamp01(u - e), ub = clamp01(u + e);
    float va = v - e, vb = v + e;
 
    V3 Pu = v3scale(v3sub(bodyPoint(ub, v, phase), bodyPoint(ua, v, phase)), 1.0f / (ub - ua));
    V3 Pv = v3scale(v3sub(bodyPoint(u, vb, phase), bodyPoint(u, va, phase)), 1.0f / (vb - va));
    V3 n = v3cross(Pu, Pv);
 
    // Outward direction = from the (bent) centreline to the surface point.
    V3 P = bodyPoint(u, v, phase);
    float off = lateralOffset(u, phase);
    V3 outward = { 0.0f, P.y, P.z - off };
    if (v3dot(n, outward) < 0.0f) n = v3scale(n, -1.0f);
 
    float len = std::sqrt(v3dot(n, n));
    if (len < 1e-6f) return { 0.0f, 1.0f, 0.0f };
    return v3scale(n, 1.0f / len);
}
 
// ---------------------------------------------------------
// Fish Class Definition
// ---------------------------------------------------------
class Fish {
public:
    float x, y, z;
    float heading, targetHeading;
    float speed;
    float tailAngle, finAngle;
    float wavePhase;
    float sizeScale;
    float r, g, b;
 
    Fish(float startX, float startY, float startZ, float scale, float red, float green, float blue) {
        x = startX; y = startY; z = startZ;
        heading = rand() % 360;
        targetHeading = heading;
        speed = 0.8f + (rand() % 10) / 10.0f;
        sizeScale = scale;
        tailAngle = 0.0f; finAngle = 0.0f; wavePhase = 0.0f;
        r = red; g = green; b = blue;
    }
 
    void update(float dt) {
        wavePhase = globalTime * speed * 6.0f;
        tailAngle = std::sin(wavePhase) * 18.0f;
        finAngle  = std::sin(globalTime * speed * 3.0f) * 20.0f;
 
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
            float diff = targetHeading - heading;
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            heading += diff * 4.0f * dt;
        } else {
            if (rand() % 100 < 1) targetHeading += (rand() % 60) - 30;
            float diff = targetHeading - heading;
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            heading += diff * 1.5f * dt;
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
        // সাদা স্ট্রাইপ (দুই জায়গায়)
        if ((u > 0.2f && u < 0.3f) || (u > 0.6f && u < 0.7f)) {
            GLfloat white[] = { 0.95f, 0.95f, 0.95f, 1.0f };
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
        } else {
            GLfloat baseCol[] = { r, g, b, 1.0f };
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, baseCol);
        }
    }
 
    // শরীর: প্যারামেট্রিক সারফেস, সঠিক নরমাল সহ
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
 
    // পিঠের পাখনা: শরীরের উপরের রেখা বরাবর বাঁকানো ফালি
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
            glVertex3f(base.x, base.y - 0.02f, base.z);                 // শরীরের ভেতরে একটু ডুবানো
            glVertex3f(base.x - 0.25f * t, base.y + h, base.z);          // পিছনের দিকে বাঁকানো ডগা
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
 
        // ---- Body ----
        GLfloat matAmbient[] = { r * 0.4f, g * 0.4f, b * 0.4f, 1.0f };
        GLfloat matDiffuse[] = { r, g, b, 1.0f };
        GLfloat matSpecular[] = { 0.6f, 0.6f, 0.6f, 1.0f };
        GLfloat matShininess[] = { 50.0f };
 
        glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);
 
        drawBody(wavePhase);
 
        // ---- Eyes (follow the body wave near the head) ----
        float dHead = lateralOffset(0.13f, wavePhase);
        drawEye(1.1f, 0.3f,  0.3f + dHead,  1.0f);
        drawEye(1.1f, 0.3f, -0.3f + dHead, -1.0f);
 
        // ---- Dorsal fin (attached to the back) ----
        setFinMaterial(r * 0.85f, g * 0.85f, b * 0.85f);
        drawDorsalFin(wavePhase);
 
        // ---- Pectoral fins (side, animated) ----
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
 
        // ---- Tail (attached to the end of the body, swings with the wave) ----
        float dTail = lateralOffset(1.0f, wavePhase);
        glPushMatrix();
        glTranslatef(-1.45f, 0.0f, dTail);
        glRotatef(tailAngle, 0.0f, 1.0f, 0.0f);
        setFinMaterial(r * 0.8f, g * 0.8f, b * 0.8f);
        drawBeautifulTail();
 
        // Tail detail lines
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
 
// ---------------------------------------------------------
// Initialization & Environment
// ---------------------------------------------------------
void initEnvironment() {
    for (int i = 0; i < 50; i++) {
        Bubble b;
        b.x = ((rand() % 100) / 100.0f * TANK_WIDTH) - TANK_WIDTH / 2;
        b.y = ((rand() % 100) / 100.0f * TANK_HEIGHT);
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
 
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
 
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    // পাখনার মতো পাতলা অংশ দুই দিক থেকেই ঠিকভাবে আলোকিত হবে
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
 
    glEnable(GL_FOG);
    GLfloat fogColor[] = {0.05f, 0.15f, 0.3f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.025f);
}
 
// ---------------------------------------------------------
// Drawing Routines
// ---------------------------------------------------------
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
 
void drawGlassTank() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
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
 
// ---------------------------------------------------------
// Main Render and Update Loop
// ---------------------------------------------------------
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
 
    // Pass 1: OPAQUE
    glDepthMask(GL_TRUE);
    drawSandBottom();
    drawPlants();
    for (auto& fish : schoolOfFish) fish.draw();
 
    // Pass 2: TRANSPARENT
    glDepthMask(GL_FALSE);
    drawBubbles();
    drawGlassTank();
    glDepthMask(GL_TRUE);
 
    glutSwapBuffers();
}
 
void update(int value) {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
 
    if (!isPaused) {
        globalTime += deltaTime;
        for (auto& fish : schoolOfFish) fish.update(deltaTime);
        for (auto& b : bubbles) {
            b.y += b.speed * deltaTime;
            if (b.y > TANK_HEIGHT + 1.0f) {
                b.y = 0.0f;
                b.x = ((rand() % 100) / 100.0f * TANK_WIDTH) - TANK_WIDTH / 2;
                b.z = ((rand() % 100) / 100.0f * TANK_DEPTH) - TANK_DEPTH / 2;
            }
        }
    }
 
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}
 
// ---------------------------------------------------------
// Input Handling
// ---------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: exit(0); break;
        case ' ': isPaused = !isPaused; break;
        case 'r': case 'R': cameraAngleX = 20.0f; cameraAngleY = 45.0f; cameraDistance = 22.0f; break;
        case '=': case '+': cameraDistance -= 1.0f; if (cameraDistance < 5.0f) cameraDistance = 5.0f; break;
        case '-': case '_': cameraDistance += 1.0f; if (cameraDistance > 50.0f) cameraDistance = 50.0f; break;
    }
    glutPostRedisplay();
}
 
void specialKeys(int key, int x, int y) {
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
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}
