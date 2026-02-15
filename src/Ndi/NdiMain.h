#pragma once

namespace Clarity {

/**
 * @brief Entry point for NDI streaming output mode
 *
 * Renders the same OutputDisplayContent.qml scene offscreen via
 * QQuickRenderControl, captures frames, and sends them over NDI.
 * No visible window is created.
 */
class NdiMain {
public:
    static int run(int argc, char* argv[]);
};

} // namespace Clarity
