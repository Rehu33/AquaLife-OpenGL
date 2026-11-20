# AquaLife

A small, self-contained 3D aquarium made with C++17, OpenGL, and FreeGLUT. The fish, plants, rocks, bubbles, light rays, and water effects are drawn in code; no artwork or other assets need to be downloaded.

## Windows setup (MSYS2 UCRT64)

These steps install the compiler and libraries locally on your PC. They do not create a GitHub repository or publish any project files.

1. Install **MSYS2** from [msys2.org](https://www.msys2.org/).
2. Open **MSYS2 UCRT64** from the Start menu (not the MSYS, MinGW64, or plain UCRT64-independent terminal).
3. Update the package database and install the toolchain:

   ```bash
   pacman -Syu
   ```

   If the update asks you to close the terminal, close it, reopen **MSYS2 UCRT64**, then continue with:

   ```bash
   pacman -Su
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-freeglut
   ```

4. Add `C:\msys64\ucrt64\bin` to your Windows **PATH**, then restart VS Code. This lets CMake find the build tools and Windows load the FreeGLUT DLL. The project's build tasks use the UCRT64 compiler and CMake directly.
5. In VS Code, choose **File > Open Folder...** and open this AquaLife project folder.

If MSYS2 was installed somewhere other than `C:\msys64`, update the compiler path in `.vscode/settings.json` and the configure command below to match your installation.

## Build and run

In VS Code, use **Terminal > Run Build Task... > AquaLife: build** (or press **Ctrl+Shift+B** with this folder open). To start the aquarium, use **Terminal > Run Task... > AquaLife: run**. If VS Code asks which build task to use, choose **AquaLife: build**.

Alternatively, open a new VS Code terminal (**Terminal > New Terminal**) and run these commands from the project folder:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/ninja.exe -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
.\build\AquaLife.exe
```

**Important:** Do not use the C/C++ extension's **build active file** task or the Code Runner **Run Code** button. Those compile `main.cpp` directly with the old `g++`, without FreeGLUT's include and link settings, which produces `GL/freeglut.h: No such file or directory`.

## Controls

| Key | Action |
| --- | --- |
| Arrow keys | Rotate the view |
| Left mouse button + drag | Rotate the view |
| Mouse wheel | Zoom in or out |
| `+` / `-` | Zoom in / out |
| Space | Pause or resume the animation |
| `R` | Reset the camera |
| `Esc` | Close the aquarium |

## Troubleshooting

- **CMake cannot find GLUT or OpenGL:** Open the **MSYS2 UCRT64** terminal and check that `mingw-w64-ucrt-x86_64-freeglut` and the UCRT64 GCC/CMake packages are installed. Confirm `C:\msys64\ucrt64\bin` is on PATH, restart VS Code, and rerun the configure command.
- **The app says `freeglut.dll` is missing:** Add `C:\msys64\ucrt64\bin` to PATH, then launch it from a new terminal.
- **A different compiler is selected:** Use the UCRT64 `g++.exe` path shown in the configure command. Do not mix MSYS2 environments (for example UCRT64 and MinGW64).

This project uses FreeGLUT's compatibility-style immediate-mode OpenGL calls for simplicity and broad driver support. A working desktop OpenGL driver is required; rendering quality and frame rate depend on the machine's graphics driver.
