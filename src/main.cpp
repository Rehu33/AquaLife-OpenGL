#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "common.h"
#include "fish.h"

// ---------- Camera and input state ----------
float cameraAngleX = 20.0f;
float cameraAngleY = 45.0f;
float cameraDistance = 22.0f;

int lastTime = 0;
int lastMouseX = 0;
int lastMouseY = 0;
bool isDraggingCamera = false;

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