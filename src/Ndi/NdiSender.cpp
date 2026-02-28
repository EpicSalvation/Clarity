// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "NdiSender.h"
#include <QDebug>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Clarity {

NdiSender::NdiSender(QObject* parent)
    : QObject(parent)
{
}

NdiSender::~NdiSender()
{
    if (m_sendInstance && m_sendDestroy) {
        m_sendDestroy(m_sendInstance);
        m_sendInstance = nullptr;
    }

    if (m_destroy) {
        m_destroy();
    }

    unloadLibrary();
}

bool NdiSender::initialize(const QString& sourceName)
{
    if (!loadLibrary()) {
        return false;
    }

    // Initialize the NDI runtime
    if (!m_initialize || !m_initialize()) {
        qCritical() << "NdiSender: NDIlib_initialize() failed";
        return false;
    }

    // Create sender
    QByteArray nameUtf8 = sourceName.toUtf8();
    NDIlib_send_create_t createDesc;
    createDesc.p_ndi_name = nameUtf8.constData();
    createDesc.clock_video = true;
    createDesc.clock_audio = false;

    m_sendInstance = m_sendCreate(&createDesc);
    if (!m_sendInstance) {
        qCritical() << "NdiSender: Failed to create NDI sender";
        return false;
    }

    qDebug() << "NdiSender: Initialized NDI sender as" << sourceName;
    return true;
}

void NdiSender::sendFrame(const uint8_t* data, int width, int height, int fps)
{
    if (!m_sendInstance || !m_sendVideo) return;

    NDIlib_video_frame_v2_t frame;
    frame.xres = width;
    frame.yres = height;
    frame.FourCC = NDIlib_FourCC_video_type_BGRA;
    frame.frame_rate_N = fps * 1000;
    frame.frame_rate_D = 1001;
    frame.picture_aspect_ratio = 0.0f;
    frame.frame_format_type = NDIlib_frame_format_type_progressive;
    frame.timecode = -1;  // Auto-generate timecode
    frame.p_data = const_cast<uint8_t*>(data);
    frame.line_stride_in_bytes = width * 4;  // BGRA = 4 bytes per pixel

    // NDI send with clock_video=true will block to maintain frame rate
    m_sendVideo(m_sendInstance, &frame);
}

int NdiSender::connectionCount() const
{
    if (!m_sendInstance || !m_getConnections) return 0;
    return m_getConnections(m_sendInstance, 0);
}

bool NdiSender::loadLibrary()
{
#ifdef Q_OS_WIN
    QString dllName = "Processing.NDI.Lib.x64.dll";

    // NDI Tools has used different env var names across versions.
    // Try them all, newest first: NDI_RUNTIME_DIR_V6..V2, then NDILIB_REDIST_FOLDER.
    QStringList envVars = {
        "NDI_RUNTIME_DIR_V6", "NDI_RUNTIME_DIR_V5", "NDI_RUNTIME_DIR_V4",
        "NDI_RUNTIME_DIR_V3", "NDI_RUNTIME_DIR_V2", "NDILIB_REDIST_FOLDER"
    };

    for (const QString& envVar : envVars) {
        QString folder = qEnvironmentVariable(envVar.toUtf8().constData());
        if (!folder.isEmpty()) {
            QString fullPath = QDir(folder).filePath(dllName);
            m_library = LoadLibraryW(reinterpret_cast<LPCWSTR>(fullPath.utf16()));
            if (m_library) {
                qDebug() << "NdiSender: Loaded DLL from" << envVar << "=" << folder;
                break;
            }
        }
    }

    // Fall back to standard DLL search path
    if (!m_library) {
        m_library = LoadLibraryW(reinterpret_cast<LPCWSTR>(dllName.utf16()));
        if (m_library) {
            qDebug() << "NdiSender: Loaded DLL from system PATH";
        }
    }

    if (!m_library) {
        qCritical() << "NdiSender: Could not load NDI runtime DLL."
                     << "Please install NDI Tools from https://ndi.video/tools/";
        return false;
    }

    // Resolve function pointers
    m_initialize = reinterpret_cast<NDIlib_initialize_fn>(
        GetProcAddress(m_library, "NDIlib_initialize"));
    m_destroy = reinterpret_cast<NDIlib_destroy_fn>(
        GetProcAddress(m_library, "NDIlib_destroy"));
    m_sendCreate = reinterpret_cast<NDIlib_send_create_fn>(
        GetProcAddress(m_library, "NDIlib_send_create"));
    m_sendDestroy = reinterpret_cast<NDIlib_send_destroy_fn>(
        GetProcAddress(m_library, "NDIlib_send_destroy"));
    m_sendVideo = reinterpret_cast<NDIlib_send_send_video_v2_fn>(
        GetProcAddress(m_library, "NDIlib_send_send_video_v2"));
    m_getConnections = reinterpret_cast<NDIlib_send_get_no_connections_fn>(
        GetProcAddress(m_library, "NDIlib_send_get_no_connections"));

    if (!m_initialize || !m_destroy || !m_sendCreate ||
        !m_sendDestroy || !m_sendVideo) {
        qCritical() << "NdiSender: Failed to resolve required NDI functions from DLL";
        unloadLibrary();
        return false;
    }

    qDebug() << "NdiSender: Loaded NDI runtime DLL successfully";
    return true;

#else
    // Linux/macOS: dlopen
    const char* libName = "libndi.so";  // Linux
    #ifdef Q_OS_MACOS
    libName = "libndi.dylib";
    #endif

    QString redistFolder = qEnvironmentVariable("NDILIB_REDIST_FOLDER");
    if (!redistFolder.isEmpty()) {
        QString fullPath = QDir(redistFolder).filePath(libName);
        m_library = dlopen(fullPath.toUtf8().constData(), RTLD_NOW);
    }

    if (!m_library) {
        m_library = dlopen(libName, RTLD_NOW);
    }

    if (!m_library) {
        qCritical() << "NdiSender: Could not load NDI runtime."
                     << "Please install NDI Tools from https://ndi.video/tools/";
        return false;
    }

    m_initialize = reinterpret_cast<NDIlib_initialize_fn>(dlsym(m_library, "NDIlib_initialize"));
    m_destroy = reinterpret_cast<NDIlib_destroy_fn>(dlsym(m_library, "NDIlib_destroy"));
    m_sendCreate = reinterpret_cast<NDIlib_send_create_fn>(dlsym(m_library, "NDIlib_send_create"));
    m_sendDestroy = reinterpret_cast<NDIlib_send_destroy_fn>(dlsym(m_library, "NDIlib_send_destroy"));
    m_sendVideo = reinterpret_cast<NDIlib_send_send_video_v2_fn>(dlsym(m_library, "NDIlib_send_send_video_v2"));
    m_getConnections = reinterpret_cast<NDIlib_send_get_no_connections_fn>(dlsym(m_library, "NDIlib_send_get_no_connections"));

    if (!m_initialize || !m_destroy || !m_sendCreate ||
        !m_sendDestroy || !m_sendVideo) {
        qCritical() << "NdiSender: Failed to resolve required NDI functions";
        unloadLibrary();
        return false;
    }

    qDebug() << "NdiSender: Loaded NDI runtime successfully";
    return true;
#endif
}

void NdiSender::unloadLibrary()
{
#ifdef Q_OS_WIN
    if (m_library) {
        FreeLibrary(m_library);
        m_library = nullptr;
    }
#else
    if (m_library) {
        dlclose(m_library);
        m_library = nullptr;
    }
#endif

    m_initialize = nullptr;
    m_destroy = nullptr;
    m_sendCreate = nullptr;
    m_sendDestroy = nullptr;
    m_sendVideo = nullptr;
    m_getConnections = nullptr;
}

} // namespace Clarity
