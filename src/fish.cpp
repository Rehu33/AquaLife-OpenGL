#include "fish.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ---------- Fish body geometry helpers ----------

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

// ---------- Fish class ----------

Fish::Fish(float startX, float startY, float startZ, float scale, float red, float green, float blue) {
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

void Fish::update(float dt) {
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

void Fish::applyColorStripe(float u) {
    if ((u > 0.2f && u < 0.3f) || (u > 0.6f && u < 0.7f)) {
        GLfloat white[] = { 0.95f, 0.95f, 0.95f, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
    } else {
        GLfloat baseCol[] = { r, g, b, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, baseCol);
    }
}

void Fish::drawBody(float phase) {
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

void Fish::drawDorsalFin(float phase) {
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

void Fish::drawCurvedFin(float scaleX, float scaleY, float scaleZ) {
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

void Fish::drawBeautifulTail() {
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

void Fish::setFinMaterial(float fr, float fg, float fb) {
    GLfloat c[] = { fr, fg, fb, 0.8f };
    GLfloat spec[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, c);
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
}

void Fish::drawEye(float ex, float ey, float ez, float side) {
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

void Fish::draw() {
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