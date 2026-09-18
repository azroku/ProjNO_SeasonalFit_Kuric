#include "Application.h"

#include <gui/WinMain.h>
#include <td/StringConverter.h>

int main(int argc, const char* argv[])
{
    Application app(argc, argv);
    app.init("EN");

    return app.run();
}
