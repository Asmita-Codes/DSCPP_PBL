#include <cstring>
#include "App.h"

int runPathfindingDemo();   

int main(int argc, char** argv) {
    bool fullscreen = true;                            
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--demo") == 0) return runPathfindingDemo();
        if (std::strcmp(argv[i], "--windowed") == 0) fullscreen = false;
    }
    App app(fullscreen);
    app.run();
    return 0;
}
