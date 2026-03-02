// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include "Core/Slide.h"
#include <QWidget>
#include <QTextEdit>
#include <QList>

namespace Clarity {

class SettingsManager;

/**
 * @brief WYSIWYG canvas widget that renders a slide background and hosts
 *        transparent QTextEdit overlays for each text zone.
 *
 * The canvas maintains 16:9 aspect ratio and scales all rendering to match
 * the design resolution (1080p). Child QTextEdit widgets are positioned
 * at fractional coordinates matching each TextZone, allowing direct text
 * editing on the rendered slide.
 *
 * For Blank template slides, a single full-area QTextEdit is used.
 * For multi-zone templates (Title, TitleBody, Scripture), one QTextEdit
 * per zone is created, each positioned and sized according to the zone's
 * fractional layout coordinates.
 */
class SlideCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit SlideCanvasWidget(SettingsManager* settings, QWidget* parent = nullptr);

    /// Load a slide into the canvas — rebuilds zone editors and repaints
    void setSlide(const Slide& slide);

    /// Collect current canvas state back into a Slide object
    Slide slide() const;

    /// Preferred 16:9 height for a given width
    int heightForWidth(int w) const override;
    bool hasHeightForWidth() const override { return true; }
    QSize sizeHint() const override;

    // --- Per-zone property setters (called by toolbar) ---
    void setZoneFontFamily(const QString& family);
    void setZoneFontSize(int size);
    void setZoneTextColor(const QColor& color);
    void setZoneHorizontalAlignment(int align); // 0=Left, 1=Center, 2=Right

    // --- List support ---
    void toggleBulletList();
    void toggleNumberedList();
    int currentListStyle() const; // 0=none, 1=bullet, 2=numbered

    // --- Slide-level setters for live preview ---
    void setBackgroundColor(const QColor& color);
    void setBackgroundType(Slide::BackgroundType type);
    void setGradientStops(const QList<GradientStop>& stops);
    void setGradientType(GradientType type);
    void setGradientAngle(int angle);
    void setRadialCenterX(double x);
    void setRadialCenterY(double y);
    void setRadialRadius(double r);
    void setBackgroundImagePath(const QString& path);
    void setBackgroundImageData(const QByteArray& data);
    void setBackgroundVideoPath(const QString& path);
    void setVideoLoop(bool loop);
    void setOverlayEnabled(bool enabled);
    void setOverlayColor(const QColor& color);
    void setOverlayBlur(int blur);
    void setDropShadowEnabled(bool enabled);
    void setDropShadowColor(const QColor& color);
    void setDropShadowOffsetX(int offset);
    void setDropShadowOffsetY(int offset);
    void setDropShadowBlur(int blur);
    void setTextContainerEnabled(bool enabled);
    void setTextContainerColor(const QColor& color);
    void setTextContainerPadding(int padding);
    void setTextContainerRadius(int radius);
    void setTextContainerBlur(int blur);
    void setTextBandEnabled(bool enabled);
    void setTextBandColor(const QColor& color);
    void setTextBandBlur(int blur);
    void setSlideTemplate(SlideTemplate tmpl);
    void setHasExplicitBackground(bool v);

    // Per-zone legibility setters (for selected zone)
    void setZoneDropShadowEnabled(bool enabled);
    void setZoneDropShadowColor(const QColor& color);
    void setZoneDropShadowOffsetX(int offset);
    void setZoneDropShadowOffsetY(int offset);
    void setZoneDropShadowBlur(int blur);
    void setZoneTextContainerEnabled(bool enabled);
    void setZoneTextContainerColor(const QColor& color);
    void setZoneTextContainerPadding(int padding);
    void setZoneTextContainerRadius(int radius);
    void setZoneTextBandEnabled(bool enabled);
    void setZoneTextBandColor(const QColor& color);

    int selectedZoneIndex() const { return m_selectedZoneIndex; }
    int zoneCount() const { return m_zoneEditors.size(); }

signals:
    /// Emitted when a zone editor receives focus (index = zone index)
    void zoneSelected(int index);

    /// Emitted when any text or property changes (for live preview updates)
    void canvasChanged();

    /// Emitted when list style changes at cursor position (0=none, 1=bullet, 2=numbered)
    void listStyleChanged(int style);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    /// Rebuild QTextEdit children based on current slide template/zones
    void rebuildZoneEditors();

    /// Reposition and resize zone editors to match current canvas size
    void updateZoneGeometry();

    /// Update font scaling for all zone editors based on canvas size
    void updateFontScaling();

    /// Compute the actual slide rect within the widget (centered, 16:9)
    QRect slideRect() const;

    /// Scale factor from 1080p design to current canvas height
    double scaleFactor() const;

    /// Build a temporary Slide reflecting the current canvas state (for painting)
    Slide buildCurrentSlide() const;

    SettingsManager* m_settings;
    Slide m_slide;                       // Current slide data (background, legibility, etc.)
    int m_selectedZoneIndex = -1;        // Currently focused zone (-1 = none)
    bool m_updatingGeometry = false;     // Re-entry guard for updateZoneGeometry

    struct ZoneEditor {
        QTextEdit* editor = nullptr;
        TextZone zone;                   // Zone layout/style metadata
    };
    QList<ZoneEditor> m_zoneEditors;
};

} // namespace Clarity
