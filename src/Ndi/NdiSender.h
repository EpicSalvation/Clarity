// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "NdiTypes.h"
#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Clarity {

/**
 * @brief NDI sender wrapper with dynamic DLL loading
 *
 * Loads the NDI runtime DLL at runtime (no link-time dependency),
 * creates an NDI sender, and sends RGBA video frames.
 *
 * The NDI runtime is located via the NDILIB_REDIST_FOLDER environment
 * variable, which is set by the NDI Tools installer.
 */
class NdiSender : public QObject {
    Q_OBJECT

public:
    explicit NdiSender(QObject* parent = nullptr);
    ~NdiSender();

    /**
     * Initialize NDI: load the DLL and create a sender.
     * @param sourceName  The NDI source name visible on the network
     * @return true on success, false if NDI runtime not found or init failed
     */
    bool initialize(const QString& sourceName);

    /**
     * Send a video frame over NDI.
     * @param data    BGRA pixel data (4 bytes per pixel)
     * @param width   Frame width in pixels
     * @param height  Frame height in pixels
     * @param fps     Target frames per second
     */
    void sendFrame(const uint8_t* data, int width, int height, int fps);

    /** @return true if the NDI sender is initialized and ready */
    bool isReady() const { return m_sendInstance != nullptr; }

    /** @return number of connected NDI receivers */
    int connectionCount() const;

private:
    bool loadLibrary();
    void unloadLibrary();

#ifdef Q_OS_WIN
    HMODULE m_library = nullptr;
#else
    void* m_library = nullptr;
#endif

    // Dynamically resolved function pointers
    NDIlib_initialize_fn m_initialize = nullptr;
    NDIlib_destroy_fn m_destroy = nullptr;
    NDIlib_send_create_fn m_sendCreate = nullptr;
    NDIlib_send_destroy_fn m_sendDestroy = nullptr;
    NDIlib_send_send_video_v2_fn m_sendVideo = nullptr;
    NDIlib_send_get_no_connections_fn m_getConnections = nullptr;

    NDIlib_send_instance_t m_sendInstance = nullptr;
};

} // namespace Clarity
