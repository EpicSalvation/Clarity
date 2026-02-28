// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

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
