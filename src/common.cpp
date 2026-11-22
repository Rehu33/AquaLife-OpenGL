#include "common.h"

const float TANK_WIDTH  = 20.0f;
const float TANK_HEIGHT = 10.0f;
const float TANK_DEPTH  = 15.0f;
const float WATER_LEVEL = TANK_HEIGHT - 0.35f;

float globalTime = 0.0f;
bool isPaused = false;

std::vector<Bubble> bubbles;
std::vector<Plant> plants;