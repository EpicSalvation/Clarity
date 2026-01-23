#include "ControlMain.h"
#include "ControlWindow.h"
#include <QApplication>

namespace Clarity {

int ControlMain::run(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Clarity");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Clarity");

    ControlWindow window;
    window.show();

    return app.exec();
}

} // namespace Clarity
