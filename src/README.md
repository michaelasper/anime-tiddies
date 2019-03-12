This should work as long as GLFW3 and GLEW are installed.  It uses GLSL 3.3, core 330 due to limitations
on OS X.  This does not currently compile on the departmental machines, because GLFW3 and GLEW are not
installed

To build and run:

~~~~
mkdir build
cd build
cmake ..
make
cd ..
build/bin/bunny
~~~~
