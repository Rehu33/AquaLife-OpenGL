#pragma once

#include "common.h"

// Look of one kind of fish (colours, body shape, stripes, fins).
struct FishSpecies {
    float body[3];          // back colour
    float belly[3];         // belly / side colour
    float fin[3];           // fin and tail colour
    float accent[3];        // stripe or lateral line colour

    float heightScale;      // body tallness (1.0 = normal)
    float widthScale;       // body thickness (1.0 = normal)
    float backStart;        // sin(v) where the back colour starts (lower = wider back)

    int   stripeCount;      // number of vertical stripes (0 = none)
    float stripeHalfWidth;  // half width of one stripe along the body (0..1)
    bool  lateralLine;      // thin accent line along the side

    float tailFork;         // 0 = round tail, 0.4 = deep fork
    float dorsalHeight;     // height of the top fin
};

// Ready-made species (inspired by the reference picture)
FishSpecies speciesGoldfish();
FishSpecies speciesTallStriped();
FishSpecies speciesYellow();
FishSpecies speciesRedLine();
FishSpecies speciesRedBack();
FishSpecies speciesBlueStriped();
FishSpecies speciesBlueYellowTail();

// ---------- Fish body geometry helpers ----------
float profileR(float u);
float lateralOffset(float u, float phase);
V3 bodyPoint(float u, float v, float phase, float heightScale = 1.0f, float widthScale = 1.0f);
V3 bodyNormal(float u, float v, float phase, float heightScale = 1.0f, float widthScale = 1.0f);

class Fish {
public:
    float x, y, z;
    float heading, targetHeading;
    float speed;
    float cruiseY, verticalPhase, turnTimer;
    float tailAngle, finAngle;
    float wavePhase;
    float sizeScale;
    FishSpecies style;

    Fish(float startX, float startY, float startZ, float scale, const FishSpecies& species);

    void update(float dt);
    void draw();

private:
    void applyVertexColor(float u, float v) const;
    void drawBody(float phase);
    void drawEyes(float phase);
    void drawDorsalFin(float phase);
    void drawAnalFin(float phase);
    void drawPelvicFins(float phase);
    void drawPectoralFins(float phase);
    void drawCurvedFin(float scaleX, float scaleY, float scaleZ);
    void drawTail();
    void setFinMaterial(float fr, float fg, float fb);
};