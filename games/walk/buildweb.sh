#!/bin/sh

# 1. Configuration
CC="emcc"
SRC="./src/walk.c"
OUT="walk.exe"
INC="-I../lib/include"
LIB_PATH="-L../lib/bin"
WLIB_PATH="-L ../lib/bin/web"
LIBS="-lrlimgui -lraylib -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32 -lstdc++"
WLIBGS="-lraylib.web"
CFLAGS="-g -Wall -Werror"
OPTFLAGS="-DPLATFORM_WEB -DDGRAPHICS_API_OPENGL_ES2 -s USE_GLFW=3 -o walk.html --shell-file ../lib/src/raylib/src/shell.html"


# 2. Build Process
echo "Compiling $SRC..."

$CC  $CFLAGS $SRC -o $OUT $INC $WLIB_PATH $WLIBGS $OPTFLAGS -s EXPORTED_RUNTIME_METHODS=['requestFullscreen']   -s TOTAL_MEMORY=671088640   

# 3. Check if build succeeded
if [ $? -eq 0 ]; then
    echo "---------------------------"
    echo "Build Successful: $OUT"
    echo "Running game..."
    echo "---------------------------"
else
    echo "Build Failed!"
    exit 1
fi