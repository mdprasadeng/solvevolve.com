* Building raylib .a file
* Using the w64devkit.exe terminal run make in raylib to generate the a file

* Buiding imgui for raylib
 * clone imgui
 * clone dearimgui bindings for c binding
 * clone raylub-extras rlImGui for raylib backend to imgui
 * compile using g++ the cpp files in rlImgui imgui dearimgui-bindings this needs including raylib src
 * g++ -std=c++11 -c *.cpp  ../imgui/*.cpp -I../imgui -I../raylib/src //run in rlImGui after copying the generate cpp file from bindings
 * ar rcs librlimgui.a *.o // create the lib
 

 * link to the libs via the build.sh file
 * in code include rlimgui header for bootstrap functions 
 * call the c methods exposed via bindings for imgui functionality

 