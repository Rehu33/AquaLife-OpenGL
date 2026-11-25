#pragma once

#include "common.h"

void initEnvironment();          // creates bubbles and plants
void updateBubbles(float dt);    // moves bubbles upward, resets at the surface

void drawSandBottom();
void drawRocks();
void drawPlants();
void drawBubbles();

// Decorative underwater light shafts.
void drawLightRays();