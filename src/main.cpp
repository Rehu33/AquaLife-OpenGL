#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "common.h"
#include "fish.h"
#include "environment.h"
#include "water.h"
#include "particles.h"

// ---------- Camera and input state ----------
float cameraAngleX = 20.0f;
float cameraAngleY = 45.0f;
float cameraDistance = 22.0f;

int lastTime = 0;
int lastMouseX = 0;
int lastMouseY = 0;
bool isDraggingCamera = false;

std::vector<Fish> schoolOfFish;

void initFish() {
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
    drawLightRays();
    drawParticles();
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
        updateBubbles(deltaTime);
        updateParticles(deltaTime);
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
    initFish();
    initParticles();
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