# Data Structure and Algorithm visualizations
Some visualizations of data structures and algorithms.

## Repo Structure
- /shell.bat sets up the visual studio compiler for use. I am not sure if the path is valid on every Windows 10 machine.
- All of the headers are in /include which includes headers for the data structures.
- All of the static libs are in /lib. It also links to opengl32.lib which is not included in this folder.
- /misc has some images but nothing in there is currently used.
- /textures also has images which aren't used.
- /zshaders has a bunch of shaders. The only ones that are used are generic.vert and generic.frag.

### /src
- build.bat creates dir /build if it doesn't exist and builds the project into there
- run.bat runs win32_main.exe in the build directory
- open.bat is used for vim. It is a simple command to open vim with the session file work.vim. It opens a predefined set of tabs. To modify the session, open the session, add, delete, or move tabs, run the command ':mks! work.vim'

### win32_main.cpp
This is the only translation unit. The other .cpp files are included in here and compiled. The code here is meant to be windows specific code (setting up the window, opengl, reading/writing files, getting input, etc) AND it sets up the data structures.

### opengl.cpp
This contains the calls to opengl to set up vertex arrays, shaders, etc.

### engine.cpp
This contains the main driver of the program. It updates the state of the data structures and it contains the code to draw them.

The main structure of the code is taken from Casey Muratori's Handmade Hero project (https://handmadehero.org). 

The main idea is that there are 2 layers: platform layer and game layer:
- Platform layer contains code for each platform that is targeted (right now only Windows). This takes care of things like file i/o, processing input, creating a window, etc. Right now it also sets up the data structures, but maybe that should be moved. There would be a separate set of .cpp files for each platform. 

- Game layer contains all of the code to update state and draw. The platform layer fills out the structs in engine.h and passes them off to the game layer to do the rest of the work. The only service that is currently used by both is opengl. The platform layer uses it to set up the vertex arrays and shaders, while engine.cpp sends updated data to the GPU and issues draw calls.


# TO USE
Right now I only have visualizations for a queue, binary tree, and insertion sort. They are all loaded into the GameState.data_structures[] struct found in engine.h before the main loop. The camera is still global so if you move it around in one view it will move for the others.

##### Navigation
- 'w'/'v' keys move through views

##### Camera
- ',' zooms in
- 's' zooms out
- left/right/up/down moves camera along x/y axis

##### Binary Tree
<img src="gifs/binary_tree.gif" width="750" height="250" />


- 'a' adds random element within (0-99 as those are the only digits supported) will not add anything if element already exists
- 'e' deletes selected element
- 'p' goes to previous node
- 's' goes to next node

##### Queue
<img src="gifs/queue.gif" width="750" height="250" />
- 'a' adds element
- 'e' deletes element

##### Insertion Sort
<img src="gifs/insertion_sort.gif" width="750" height="250" />
- 's' starts sort



