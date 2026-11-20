# 🐠 AquaLife: An Interactive 3D Aquarium Animation Using OpenGL

AquaLife is an interactive 3D aquarium written in C++ with OpenGL and FreeGLUT. Five fish swim inside a glass tank over a sandy floor, with animated water, rising bubbles, swaying plants, and fog for depth. The whole scene is drawn in code, so no image or model files are needed.

This project was built for the Computer Graphics Lab (CSE-4742) at the International Islamic University Chittagong, Spring 2026.

## Screenshots

### Water surface and ripples

![Water ripple 1](Screenshots/1.png)

![Water ripple 2](Screenshots/2.png)

## ✨ Features

- **Glass tank** with transparent walls and an edge outline.
- **Sandy floor** and a blue underwater background with exponential fog.
- **Five 3D fish** in different colours. Each fish has a parametric body, white stripes, eyes, a dorsal fin, pectoral fins, and a tail.
- **Fish movement:** tail and fin animation, random wandering, and turning away from the tank walls.
- **Animated water surface** built from two overlapping sine waves, with per-vertex normals for lighting highlights.
- **50 bubbles** that rise from the floor with side-to-side wobble and restart when they reach the surface.
- **20 swaying plants** drawn as chains of cylinders.
- **Camera controls:** rotate, zoom, pause, and reset.

## Requirements

- Windows or Linux
- A C++ compiler that supports C++11 or later (for example, MinGW-w64 g++ on Windows)
- FreeGLUT and OpenGL development libraries
- A graphics driver with OpenGL support

## Build and Run

### Windows (MinGW-w64)

Install FreeGLUT for MinGW, then run these commands in the project folder:

```cmd
g++ src/main.cpp -o AquaLife -lfreeglut -lopengl32 -lglu32
AquaLife.exe
```

If Windows reports that `freeglut.dll` is missing, copy `freeglut.dll` from the FreeGLUT `bin` folder into the same folder as `AquaLife.exe`.

### Linux (Ubuntu or Debian)

```bash
sudo apt install freeglut3-dev
g++ src/main.cpp -o AquaLife -lglut -lGL -lGLU
./AquaLife
```

## 🎮 Controls

| Key | Action |
| --- | --- |
| Up / Down arrow | Tilt the camera up or down |
| Left / Right arrow | Rotate the camera around the tank |
| `+` or `=` | Zoom in |
| `-` or `_` | Zoom out |
| Space | Pause or resume the animation |
| `R` | Reset the camera |
| `Esc` | Close the program |

## Project Structure

```text
AquaLife-OpenGL/
├── src/
│   └── main.cpp
├── Screenshots/
│   ├── 1.png
│   └── 2.png
├── CMakeLists.txt
├── README.md
└── .gitignore
```

## Planned Work

- Rocks and small decorative objects on the floor
- Underwater light rays and floating particles
- Day and night lighting modes
- Placing the aquarium in a room or roadside setting
- Sound effects with a sound on/off control
- Fish information shown when a fish is selected

## Technologies

| Technology | Purpose |
| --- | --- |
| C++ | Application logic |
| OpenGL | 3D rendering, lighting, fog, and blending |
| FreeGLUT | Window creation, keyboard input, and animation timer |

## Student Information

| Field | Details |
| --- | --- |
| Name | Rehab Binte Alamgir |
| Student ID | C233455 |
| Course | CSE-4742, Computer Graphics Lab |
| Semester | 7th, Spring 2026 |
| Instructor | Sultana Tasnim Jahan |
| Institution | International Islamic University Chittagong (IIUC) |

## Status

Ongoing. The project is developed in stages, and new visual features are added in each stage.