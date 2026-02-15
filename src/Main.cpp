/**
 * Clarity - Main Entry Point
 *
 * Routes execution to different modes based on command-line arguments:
 * - No args or --control: Control mode (Qt Widgets)
 * - --output: Output display mode (Qt Quick)
 * - --confidence: Confidence monitor mode (Qt Quick)
 */

#include "Control/ControlMain.h"
#include "Output/OutputMain.h"
#include "Confidence/ConfidenceMain.h"
#ifdef CLARITY_NDI_ENABLED
#include "Ndi/NdiMain.h"
#endif
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    // Parse command-line arguments early
    // We need to determine which mode before initializing QApplication
    bool isOutput = false;
    bool isConfidence = false;
    bool isNdi = false;

    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "--output") {
            isOutput = true;
        } else if (arg == "--confidence") {
            isConfidence = true;
        } else if (arg == "--ndi") {
            isNdi = true;
        }
    }

    // Route to appropriate mode
    if (isOutput) {
        qDebug() << "Starting Clarity in OUTPUT mode";
        return Clarity::OutputMain::run(argc, argv);
    } else if (isConfidence) {
        qDebug() << "Starting Clarity in CONFIDENCE mode";
        return Clarity::ConfidenceMain::run(argc, argv);
#ifdef CLARITY_NDI_ENABLED
    } else if (isNdi) {
        qDebug() << "Starting Clarity in NDI mode";
        return Clarity::NdiMain::run(argc, argv);
#endif
    } else {
        qDebug() << "Starting Clarity in CONTROL mode";
        return Clarity::ControlMain::run(argc, argv);
    }
}
