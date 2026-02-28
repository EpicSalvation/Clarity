// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include <QApplication>

namespace Clarity {

/**
 * @brief Entry point for control mode
 *
 * Initializes the control application with Qt Widgets UI
 */
class ControlMain {
public:
    static int run(int argc, char* argv[]);
};

} // namespace Clarity
