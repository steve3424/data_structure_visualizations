# Data Structure and Algorithm visualizations
Some visualizations of data structures and algorithms.

## Repo Structure
- /shell.bat - sets up the visual studio compiler for use. I am not sure if the path is valid on every Windows 10 machine.
- /include   - Some 3rd party headers.
- /lib       - Libraries needed to link to. **Also links to opengl32.lib**
- /logs      - stderr is redirected to a text file which I use to output any opengl errors.
- /src       - all source code and a view .bat files for convenience.
- /textures  - right now only used for the single background image.
- /zshaders  - shaders for the basic objects used and for a background image. (It is named zshaders so I can easily navigate to src by pressing 's' and tabbing on the command line)

## Project structure
The basic structure for the project was learned from Casey Muratori's [Handmade Hero](https://handmadehero.org) project.

There are 3 primary files that drive the project:
1. /src/win32_main.cpp - This is the platform layer. This handles initializing opengl, file io, etc. and provides it's interface in /src/win32_main.h. Any other platform layer could also be set up. In that case it would probably make sense to provide a uniform interface for platform related stuff that the other parts of the code might need.

2. /src/engine.cpp - This is the main engine. The main function is GameUpdateAndRender(). It handles a few things such as navigating the different visualizations that are set up, moving the camera, and generating some basic objects. It provides an interface in /src/engine.h which contains some basic structures that might be used by the different visualizations e.g. cube, digit, background.

3. *visualization files* - Each visualization that is added would be in a separate .cpp file. This file would take care of getting buffers on the gpu, setting up whatever geometry/data it needs, updating the visualization, making draw calls, etc.

**To add a visualization**

Create the visualization in a .cpp file with an interface that can be called from /src/engine.cpp. Add an enum value for it in /src/engine.h. Create a switch case in GameUpdateAndRender() in /src/engine.cpp so it can be navigated to. Include the .cpp file in /src/win32_main.cpp above engine.cpp

## Build structure
The build structure was also a technique used in the [Handmade Hero](https://handmadehero.org) project. The only file that gets built is the platform file. This file includes all of the headers and .cpp files. The structure can be seen at the top of /src/win32_main.cpp:
```
#include "engine.h"
#include "opengl.cpp"
#include "insertion_sort.cpp"
#include "engine.cpp"
```
engine.cpp is at the bottom so it can see and use everything in the visualization files which would be included above. Any visualization file that needs the engine structs/utilities or platform structs/utilites would include /src/engine.h or /src/win32_main.h. 

# TO USE
The input keys are only setup for dvorak right now

##### Navigation
- 'w'/'v' keys move through views

##### Camera
- ',' zoom in
- 'o' zoom out
- left/right/up/down moves camera along x/y axis

##### Insertion Sort
- '0-9' adjust speed setting
- 'p' pause/unpause animation
- 's' start animation OR return to original state if animation finished
