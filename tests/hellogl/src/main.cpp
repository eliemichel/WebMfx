#include <OpenGL>

#include "Ui/App.h"

#include <Logger.h>
#include <Ui/Window.h>

#include <cstdlib>
#include <memory>

int main(int, char *[]) {
    auto window = std::make_shared<Window>(1200, 700, "OpenMfx Player");
    
    std::unique_ptr<BaseApp> app = std::make_unique<App>(window);

    while (window->isValid() && !window->shouldClose()) {
        window->pollEvents();

        app->update();
        app->render();

        window->swapBuffers();
    }

    return EXIT_SUCCESS;
}
