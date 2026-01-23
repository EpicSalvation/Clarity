#pragma once

#include <QGuiApplication>

namespace Clarity {

/**
 * @brief Entry point for output display mode
 *
 * Initializes the output display with Qt Quick/QML
 */
class OutputMain {
public:
    static int run(int argc, char* argv[]);
};

} // namespace Clarity
