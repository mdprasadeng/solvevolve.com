#!/bin/sh

# 1. Configuration
CC="gcc"
SRC="./src/walk.c"
OUT="walk.exe"
INC="-I../lib/include"
LIB_PATH="-L../lib/bin"
LIBS="-lrlimgui -lraylib -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32 -lstdc++"
CFLAGS="-g -Wall -Werror "

# 2. Build Process
echo "Compiling $SRC..."

$CC -DPLATFORM_DESKTOP $CFLAGS$SRC -o $OUT $INC $LIB_PATH $LIBS

# 3. Check if build succeeded
if [ $? -eq 0 ]; then
    echo "---------------------------"
    echo "Build Successful: $OUT"
    echo "Running game..."
    echo "---------------------------"
    gdb ./$OUT
else
    echo "Build Failed!"
    exit 1
fi