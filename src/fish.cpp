#include "fish.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ---------- small helpers ----------

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static float smooth(float t) { t = clamp01(t); return t * t * (3.0f - 2.0f * t); }

static void setRGB(float* dst, float r, float g, float b) {
    dst[0] = r; dst[1] = g; dst[2] = b;
}

// Default look, individual species change what they need.
static FishSpecies baseSpecies() {
    FishSpecies s;
    setRGB(s.body,   0.6f, 0.7f, 0.9f);
    setRGB(s.belly,  0.9f, 0.92f, 0.95f);
    setRGB(s.fin,    0.7f, 0.78f, 0.9f);
    setRGB(s.accent, 0.2f, 0.3f, 0.6f);
    s.heightScale = 1.0f;
    s.widthScale = 1.0f;
    s.backStart = -0.2f;
    s.stripeCount = 0;
    s.stripeHalfWidth = 0.03f;
    s.lateralLine = false;
    s.tailFork = 0.3f;
    s.dorsalHeight = 0.55f;
    return s;
}

// ---------- species presets ----------

FishSpecies speciesGoldfish() {
    FishSpecies s = baseSpecies();
    setRGB(s.body,   1.00f, 0.42f, 0.05f);
    setRGB(s.belly,  0.98f, 0.96f, 0.92f);
    setRGB(s.fin,    1.00f, 0.62f, 0.30f);
    setRGB(s.accent, 1.00f, 1.00f, 1.00f);
    s.backStart = -0.1f;
    s.stripeCount = 1;            // one wide white patch
    s.stripeHalfWidth = 0.14f;
    s.tailFork = 0.15f;
    s.dorsalHeight = 0.55f;
    return s;
}

FishSpecies speciesTallStriped() {
    FishSpecies s = baseSpecies();
    setRGB(s.body,   0.55f, 0.68f, 0.88f);
    setRGB(s.belly,  0.88f, 0.92f, 0.97f);
    setRGB(s.fin,    0.70f, 0.78f, 0.92f);
    setRGB(s.accent, 0.22f, 0.38f, 0.70f);
    s.heightScale = 1.35f;
    s.widthScale = 0.80f;
    s.backStart = -0.3f;
    s.stripeCount = 6;
    s.stripeHalfWidth = 0.028f;
    s.tailFork = 0.30f;
    s.dorsalHeight = 0.85f;
    return s;
}

FishSpecies speciesYellow() {
    FishSpecies s = baseSpecies();
    setRGB(s.body,   0.85f, 0.82f, 0.15f);
    setRGB(s.belly,  0.97f, 0.95f, 0.60f);
    setRGB(s.fin,    0.95f, 0.85f, 0.25f);
    s.heightScale = 0.85f;
    s.widthScale = 0.85f;
    s.tailFork = 0.30f;
    s.dorsalHeight = 0.40f;
    return s;
}

FishSpecies speciesRedLine() {
    FishSpecies s = baseSpecies();
    setRGB(s.body,   0.72f, 0.78f, 0.84f);
    setRGB(s.belly,  0.96f, 0.96f, 0.97f);
    setRGB(s.fin,    0.88f, 0.55f, 0.45f);
    setRGB(s.accent, 0.90f, 0.15f, 0.08f);
    s.heightScale = 0.80f;
    s.widthScale = 0.80f;
    s.lateralLine = true;
    s.tailFork = 0.35f;
    s.dorsalHeight = 0.45f;
    return s;
}

FishSpecies speciesRedBack() {
    FishSpecies s = baseSpecies();
    setRGB(s.body,   0.90f, 0.25f, 0.08f);
    setRGB(s.belly,  0.55f, 0.72f, 0.95f);
    setRGB(s.fin,    0.85f, 0.78f, 0.78f);
    s.backStart = 0.30f;
    s.tailFork = 0.30f;
    s.dorsalHeight = 0.60f;
    return s;
}

FishSpecies speciesBlueStriped() {
    FishSpecies s = baseSpecies();
    setRGB(s.body,   0.62f, 0.75f, 0.95f);
    setRGB(s.belly,  0.92f, 0.95f, 1.00f);
    setRGB(s.fin,    0.70f, 0.80f, 0.95f);
    setRGB(s.accent, 0.20f, 0.30f, 0.55f);
    s.heightScale = 1.05f;
    s.widthScale = 0.85f;
    s.stripeCount = 5;
    s.stripeHalfWidth = 0.035f;
    s.tailFork = 0.30f;
    s.dorsalHeight = 0.60f;
    return s;
}

FishSpecies speciesBlueYellowTail() {
    FishSpecies s = baseSpecies();
    setRGB(s.body,   0.22f, 0.42f, 0.92f);
    setRGB(s.belly,  0.72f, 0.82f, 0.96f);
    setRGB(s.fin,    0.95f, 0.85f, 0.20f);
    s.heightScale = 0.95f;
    s.widthScale = 0.90f;
    s.tailFork = 0.35f;
    s.dorsalHeight = 0.50f;
    return s;
}

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

V3 bodyPoint(float u, float v, float phase, float heightScale, float widthScale) {
    float R = profileR(u);
    float sv = std::sin(v);
    float cv = std::cos(v);

    float x = 1.5f - 3.0f * u;
    float yScale = 1.15f + 0.15f * sv;
    float y = R * yScale * sv * heightScale;
    float z = R * 0.45f * cv * widthScale + lateralOffset(u, phase);
    return { x, y, z };
}

V3 bodyNormal(float u, float v, float phase, float heightScale, float widthScale) {
    const float e = 0.001f;
    float ua = clamp01(u - e), ub = clamp01(u + e);
    float va = v - e, vb = v + e;

    V3 Pu = v3scale(v3sub(bodyPoint(ub, v, phase, heightScale, widthScale),
                          bodyPoint(ua, v, phase, heightScale, widthScale)), 1.0f / (ub - ua));
    V3 Pv = v3scale(v3sub(bodyPoint(u, vb, phase, heightScale, widthScale),
                          bodyPoint(u, va, phase, heightScale, widthScale)), 1.0f / (vb - va));
    V3 n = v3cross(Pu, Pv);

    V3 P = bodyPoint(u, v, phase, heightScale, widthScale);
    float off = lateralOffset(u, phase);
    V3 outward = { 0.0f, P.y, P.z - off };
    if (v3dot(n, outward) < 0.0f) n = v3scale(n, -1.0f);

    float len = std::sqrt(v3dot(n, n));
    if (len < 1e-6f) return { 0.0f, 1.0f, 0.0f };
    return v3scale(n, 1.0f / len);
}

// Tail outline: length at angle a (a from -1 to 1). The middle is shorter = fork.
static float tailLength(float a, float fork) {
    float edge = 1.0f - std::fabs(a);
    return 1.25f * (1.0f - fork * edge * edge);
}

// ---------- Fish class ----------

Fish::Fish(float startX, float startY, float startZ, float scale, const FishSpecies& species) {
    x = startX; y = startY; z = startZ;
    heading = rand() % 360;
    targetHeading = heading;
    speed = 0.8f + (rand() % 10) / 10.0f;
    cruiseY = startY;
    verticalPhase = (rand() % 628) / 100.0f;
    turnTimer = 2.0f + (rand() % 300) / 100.0f;
    sizeScale = scale;
    tailAngle = 0.0f; finAngle = 0.0f; wavePhase = 0.0f;
    style = species;
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

// Colour of one body vertex: back-to-belly gradient + stripes + side line.
// u = position along the body (0 head, 1 tail), v = angle around the body.
void Fish::applyVertexColor(float u, float v) const {
    float sv = std::sin(v);   // +1 = back, -1 = belly

    float dorsal = smooth((sv - style.backStart) / 0.5f);

    float c[3];
    for (int k = 0; k < 3; k++) c[k] = lerpf(style.belly[k], style.body[k], dorsal);

    // vertical stripes
    for (int s = 0; s < style.stripeCount; s++) {
        float center = (style.stripeCount == 1)
            ? 0.50f
            : 0.24f + 0.52f * s / (float)(style.stripeCount - 1);
        float m = 1.0f - std::fabs(u - center) / style.stripeHalfWidth;
        if (m > 0.0f) {
            m = smooth(m) * 0.85f;
            for (int k = 0; k < 3; k++) c[k] = lerpf(c[k], style.accent[k], m);
        }
    }

    // thin line along the side
    if (style.lateralLine) {
        float m = smooth(1.0f - std::fabs(sv) / 0.16f);
        float fade = clamp01(u / 0.15f) * clamp01((0.92f - u) / 0.15f);
        m *= fade * 0.9f;
        for (int k = 0; k < 3; k++) c[k] = lerpf(c[k], style.accent[k], m);
    }

    glColor3f(c[0], c[1], c[2]);
}

void Fish::drawBody(float phase) {
    const int slices = 60;
    const int stacks = 26;

    // Per-vertex colours drive the material only while the body is drawn.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    for (int i = 0; i < slices; i++) {
        float u1 = (float)i / slices;
        float u2 = (float)(i + 1) / slices;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= stacks; j++) {
            float v = (float)j / stacks * 2.0f * (float)M_PI;

            V3 n1 = bodyNormal(u1, v, phase, style.heightScale, style.widthScale);
            V3 p1 = bodyPoint(u1, v, phase, style.heightScale, style.widthScale);
            applyVertexColor(u1, v);
            glNormal3f(n1.x, n1.y, n1.z);
            glVertex3f(p1.x, p1.y, p1.z);

            V3 n2 = bodyNormal(u2, v, phase, style.heightScale, style.widthScale);
            V3 p2 = bodyPoint(u2, v, phase, style.heightScale, style.widthScale);
            applyVertexColor(u2, v);
            glNormal3f(n2.x, n2.y, n2.z);
            glVertex3f(p2.x, p2.y, p2.z);
        }
        glEnd();
    }

    glDisable(GL_COLOR_MATERIAL);
}

void Fish::drawEyes(float phase) {
    GLfloat eyeWhite[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat eyeGold[]   = { 0.95f, 0.78f, 0.25f, 1.0f };
    GLfloat eyeBlack[]  = { 0.03f, 0.03f, 0.03f, 1.0f };
    GLfloat noSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, noSpecular);

    // Eye positions sit on the body surface, so they follow the body shape.
    V3 eyePos[2] = {
        bodyPoint(0.12f, 0.30f, phase, style.heightScale, style.widthScale),
        bodyPoint(0.12f, (float)M_PI - 0.30f, phase, style.heightScale, style.widthScale)
    };
    float side[2] = { 1.0f, -1.0f };

    for (int i = 0; i < 2; i++) {
        glPushMatrix();
        glTranslatef(eyePos[i].x, eyePos[i].y, eyePos[i].z);

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, eyeWhite);
        glutSolidSphere(0.16, 16, 16);

        glTranslatef(0.04f, 0.0f, 0.06f * side[i]);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, eyeGold);
        glutSolidSphere(0.12, 16, 16);

        glTranslatef(0.04f, 0.0f, 0.03f * side[i]);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, eyeBlack);
        glutSolidSphere(0.075, 16, 16);

        glTranslatef(0.03f, 0.03f, 0.03f * side[i]);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, eyeWhite);
        glutSolidSphere(0.022, 8, 8);
        glPopMatrix();
    }
}

void Fish::setFinMaterial(float fr, float fg, float fb) {
    GLfloat c[] = { fr, fg, fb, 0.88f };
    GLfloat spec[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    // FRONT_AND_BACK so thin fins keep their colour from both sides
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
}

// Top fin: rises quickly to a pointed peak, then slopes back.
void Fish::drawDorsalFin(float phase) {
    const int steps = 20;
    const float uStart = 0.18f, uEnd = 0.82f;

    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0f, 0.0f, 1.0f);
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float u = uStart + (uEnd - uStart) * t;
        V3 base = bodyPoint(u, (float)M_PI * 0.5f, phase, style.heightScale, style.widthScale);

        float shape = (t < 0.30f) ? (t / 0.30f) : std::pow((1.0f - t) / 0.70f, 1.2f);
        float h = style.dorsalHeight * shape;

        glVertex3f(base.x, base.y - 0.02f, base.z);
        glVertex3f(base.x - 0.35f * h, base.y + h, base.z);   // swept back
    }
    glEnd();
}

// Small fin under the rear of the body.
void Fish::drawAnalFin(float phase) {
    const int steps = 10;
    const float uStart = 0.62f, uEnd = 0.82f;

    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.0f, 0.0f, 1.0f);
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float u = uStart + (uEnd - uStart) * t;
        V3 base = bodyPoint(u, (float)M_PI * 1.5f, phase, style.heightScale, style.widthScale);
        float h = 0.32f * std::sin((float)M_PI * t);

        glVertex3f(base.x, base.y + 0.02f, base.z);
        glVertex3f(base.x - 0.15f * t, base.y - h, base.z);
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

// Two small fins hanging below the front of the body.
void Fish::drawPelvicFins(float phase) {
    V3 belly = bodyPoint(0.27f, (float)M_PI * 1.5f, phase, style.heightScale, style.widthScale);
    float side[2] = { 1.0f, -1.0f };

    for (int i = 0; i < 2; i++) {
        glPushMatrix();
        glTranslatef(belly.x, belly.y + 0.05f, belly.z + 0.18f * style.widthScale * side[i]);
        glRotatef(finAngle * 0.5f * side[i], 1.0f, 0.0f, 0.0f);
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);   // hang downward
        drawCurvedFin(0.45f, 0.35f, 0.1f);
        glPopMatrix();
    }
}

// Side fins, placed on the body surface.
void Fish::drawPectoralFins(float phase) {
    V3 left  = bodyPoint(0.33f, -0.30f, phase, style.heightScale, style.widthScale);
    V3 right = bodyPoint(0.33f, (float)M_PI + 0.30f, phase, style.heightScale, style.widthScale);

    glPushMatrix();
    glTranslatef(left.x, left.y, left.z);
    glRotatef(finAngle, 1.0f, 0.0f, 0.0f);
    glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);
    drawCurvedFin(0.7f, 0.5f, 0.1f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(right.x, right.y, right.z);
    glRotatef(-finAngle, 1.0f, 0.0f, 0.0f);
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    drawCurvedFin(0.7f, 0.5f, 0.1f);
    glPopMatrix();
}

// Forked tail with thin dark rays.
void Fish::drawTail() {
    const int n = 16;

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= n; i++) {
        float a = -1.0f + 2.0f * i / n;
        float len = tailLength(a, style.tailFork);
        glVertex3f(-len * std::cos(a), len * std::sin(a), 0.0f);
    }
    glEnd();

    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_LINES);
    for (int i = 1; i < n; i++) {
        float a = -1.0f + 2.0f * i / n;
        float len = tailLength(a, style.tailFork) * 0.92f;
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(-len * std::cos(a), len * std::sin(a), 0.0f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void Fish::draw() {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(heading, 0.0f, 1.0f, 0.0f);
    glScalef(sizeScale, sizeScale, sizeScale);

    GLfloat matSpecular[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);

    drawBody(wavePhase);
    drawEyes(wavePhase);

    setFinMaterial(style.fin[0], style.fin[1], style.fin[2]);
    drawDorsalFin(wavePhase);
    drawAnalFin(wavePhase);
    drawPelvicFins(wavePhase);
    drawPectoralFins(wavePhase);

    float dTail = lateralOffset(1.0f, wavePhase);
    glPushMatrix();
    glTranslatef(-1.45f, 0.0f, dTail);
    glRotatef(tailAngle, 0.0f, 1.0f, 0.0f);
    drawTail();
    glPopMatrix();

    glPopMatrix();
}