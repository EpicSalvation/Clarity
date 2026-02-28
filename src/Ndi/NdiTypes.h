// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

/**
 * Minimal NDI type definitions for dynamic loading
 *
 * These definitions mirror the subset of the NDI SDK needed to create a sender
 * and send video frames. By defining them here, Clarity can build without the
 * NDI SDK installed — the actual NDI runtime DLL is loaded at runtime.
 *
 * Based on NDI SDK 6.x (Processing.NDI.Lib.x64.dll)
 */

#include <cstdint>

// NDI boolean type
using NDIlib_bool = bool;

// Opaque handle for NDI sender instance
using NDIlib_send_instance_t = void*;

// Frame type returned by NDIlib_send_capture
enum NDIlib_frame_type_e {
    NDIlib_frame_type_none = 0,
    NDIlib_frame_type_video = 1,
    NDIlib_frame_type_audio = 2,
    NDIlib_frame_type_metadata = 3,
    NDIlib_frame_type_error = 4,
    NDIlib_frame_type_status_change = 100
};

// FourCC pixel format codes
enum NDIlib_FourCC_video_type_e : uint32_t {
    NDIlib_FourCC_video_type_UYVY = 0x59565955,  // 'UYVY'
    NDIlib_FourCC_video_type_BGRA = 0x41524742,  // 'BGRA'
    NDIlib_FourCC_video_type_BGRX = 0x58524742,  // 'BGRX'
    NDIlib_FourCC_video_type_RGBA = 0x41424752,  // 'RGBA'
    NDIlib_FourCC_video_type_RGBX = 0x58424752   // 'RGBX'
};

// Frame format type (progressive, interlaced, field-based)
enum NDIlib_frame_format_type_e {
    NDIlib_frame_format_type_progressive = 1,
    NDIlib_frame_format_type_interleaved = 0,
    NDIlib_frame_format_type_field_0 = 2,
    NDIlib_frame_format_type_field_1 = 3
};

// NDI sender creation properties
struct NDIlib_send_create_t {
    const char* p_ndi_name = nullptr;    // Source name visible on the network
    const char* p_groups = nullptr;      // NDI groups (nullptr = default)
    NDIlib_bool clock_video = true;      // Use video clock for rate limiting
    NDIlib_bool clock_audio = true;      // Use audio clock for rate limiting
};

// Video frame descriptor
struct NDIlib_video_frame_v2_t {
    int xres = 0;                        // Width in pixels
    int yres = 0;                        // Height in pixels
    NDIlib_FourCC_video_type_e FourCC = NDIlib_FourCC_video_type_BGRA;
    int frame_rate_N = 30000;            // Frame rate numerator
    int frame_rate_D = 1001;             // Frame rate denominator
    float picture_aspect_ratio = 0.0f;   // 0 = square pixels (xres/yres)
    NDIlib_frame_format_type_e frame_format_type = NDIlib_frame_format_type_progressive;
    int64_t timecode = -1;               // -1 = auto (synthesized from clock)
    uint8_t* p_data = nullptr;           // Pointer to pixel data
    int line_stride_in_bytes = 0;        // Bytes per scanline (0 = auto)
    const char* p_metadata = nullptr;    // Optional XML metadata
    int64_t timestamp = 0;               // Timestamp (0 = ignore)
};

// Function pointer types for dynamically loaded NDI functions
using NDIlib_initialize_fn = NDIlib_bool(*)();
using NDIlib_destroy_fn = void(*)();
using NDIlib_send_create_fn = NDIlib_send_instance_t(*)(const NDIlib_send_create_t*);
using NDIlib_send_destroy_fn = void(*)(NDIlib_send_instance_t);
using NDIlib_send_send_video_v2_fn = void(*)(NDIlib_send_instance_t, const NDIlib_video_frame_v2_t*);
using NDIlib_send_get_no_connections_fn = int(*)(NDIlib_send_instance_t, uint32_t);
