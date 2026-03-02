// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "SlideCanvasWidget.h"
#include "Core/SlidePreviewRenderer.h"
#include "Core/SettingsManager.h"
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextList>
#include <QAbstractTextDocumentLayout>

namespace Clarity {

// Check if a QTextDocument contains any list blocks
static bool documentHasLists(const QTextDocument* doc)
{
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        if (block.textList())
            return true;
    }
    return false;
}

// Extract clean structural HTML from an editor's document, keeping only
// list tags (<ul>/<ol>/<li>) and paragraph tags with text content.
// No inline styles are emitted — the renderer applies font/color/size via CSS.
// Alignment (0=left, 1=center, 2=right) is encoded as inline style on each element.
static QString extractListHtml(QTextEdit* editor, int horizontalAlignment)
{
    const QTextDocument* doc = editor->document();
    QString html;
    QTextList* currentList = nullptr;

    QString alignStr = (horizontalAlignment == 0) ? QStringLiteral("left")
                     : (horizontalAlignment == 2) ? QStringLiteral("right")
                     : QStringLiteral("center");
    QString alignAttr = QStringLiteral(" align='") + alignStr
                      + QStringLiteral("' style='text-align:") + alignStr + QStringLiteral(";'");

    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        QTextList* list = block.textList();

        // Close previous list if we've left it
        if (currentList && currentList != list) {
            QTextListFormat::Style style = currentList->format().style();
            html += (style == QTextListFormat::ListDecimal
                     || style == QTextListFormat::ListLowerAlpha
                     || style == QTextListFormat::ListUpperAlpha)
                    ? QStringLiteral("</ol>") : QStringLiteral("</ul>");
            currentList = nullptr;
        }

        if (list) {
            // Open new list if needed
            if (currentList != list) {
                QTextListFormat::Style style = list->format().style();
                html += (style == QTextListFormat::ListDecimal
                         || style == QTextListFormat::ListLowerAlpha
                         || style == QTextListFormat::ListUpperAlpha)
                        ? QStringLiteral("<ol") + alignAttr + QStringLiteral(">")
                        : QStringLiteral("<ul") + alignAttr + QStringLiteral(">");
                currentList = list;
            }
            html += QStringLiteral("<li") + alignAttr + QStringLiteral(">") + block.text().toHtmlEscaped() + QStringLiteral("</li>");
        } else {
            // Regular paragraph
            html += QStringLiteral("<p") + alignAttr + QStringLiteral(">") + block.text().toHtmlEscaped() + QStringLiteral("</p>");
        }
    }

    // Close any trailing open list
    if (currentList) {
        QTextListFormat::Style style = currentList->format().style();
        html += (style == QTextListFormat::ListDecimal
                 || style == QTextListFormat::ListLowerAlpha
                 || style == QTextListFormat::ListUpperAlpha)
                ? QStringLiteral("</ol>") : QStringLiteral("</ul>");
    }

    return html;
}

SlideCanvasWidget::SlideCanvasWidget(SettingsManager* settings, QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
{
    setMinimumSize(320, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

int SlideCanvasWidget::heightForWidth(int w) const
{
    return w * 9 / 16;
}

QSize SlideCanvasWidget::sizeHint() const
{
    return QSize(640, 360);
}

QRect SlideCanvasWidget::slideRect() const
{
    int w = width();
    int h = height();
    int slideW = w;
    int slideH = w * 9 / 16;
    if (slideH > h) {
        slideH = h;
        slideW = h * 16 / 9;
    }
    int x = (w - slideW) / 2;
    int y = (h - slideH) / 2;
    return QRect(x, y, slideW, slideH);
}

double SlideCanvasWidget::scaleFactor() const
{
    return slideRect().height() / 1080.0;
}

void SlideCanvasWidget::setSlide(const Slide& slide)
{
    m_slide = slide;
    rebuildZoneEditors();
    update();
}

Slide SlideCanvasWidget::slide() const
{
    return buildCurrentSlide();
}

Slide SlideCanvasWidget::buildCurrentSlide() const
{
    Slide s = m_slide;

    if (m_slide.slideTemplate() != SlideTemplate::Blank && !m_zoneEditors.isEmpty()) {
        QList<TextZone> zones;
        for (int i = 0; i < m_zoneEditors.size(); ++i) {
            TextZone z = m_zoneEditors[i].zone;
            z.text = m_zoneEditors[i].editor->toPlainText();
            if (documentHasLists(m_zoneEditors[i].editor->document()))
                z.richText = extractListHtml(m_zoneEditors[i].editor, z.horizontalAlignment);
            else
                z.richText.clear();
            zones.append(z);
        }
        s.setTextZones(zones);
        s.setText(QString());
    } else if (!m_zoneEditors.isEmpty()) {
        s.setText(m_zoneEditors[0].editor->toPlainText());
        if (documentHasLists(m_zoneEditors[0].editor->document()))
            s.setRichText(extractListHtml(m_zoneEditors[0].editor, m_zoneEditors[0].zone.horizontalAlignment));
        else
            s.setRichText(QString());
        s.setTextZones({});
    }

    return s;
}

void SlideCanvasWidget::rebuildZoneEditors()
{
    for (auto& ze : m_zoneEditors) {
        delete ze.editor;
    }
    m_zoneEditors.clear();
    m_selectedZoneIndex = -1;

    if (m_slide.slideTemplate() != SlideTemplate::Blank && m_slide.hasTextZones()) {
        const auto zones = m_slide.textZones();
        for (int i = 0; i < zones.size(); ++i) {
            const auto& zone = zones[i];
            QTextEdit* editor = new QTextEdit(this);
            editor->setFrameShape(QFrame::NoFrame);
            editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            editor->setStyleSheet(
                "QTextEdit { background: transparent; border: none; }"
            );
            if (!zone.richText.isEmpty())
                editor->setHtml(zone.richText);
            else
                editor->setPlainText(zone.text);
            editor->setPlaceholderText(zone.id + "...");
            // Remove default document margin so text aligns with our painted positions
            editor->document()->setDocumentMargin(0);

            connect(editor, &QTextEdit::textChanged, this, [this, i]() {
                if (i < m_zoneEditors.size()) {
                    m_zoneEditors[i].zone.text = m_zoneEditors[i].editor->toPlainText();
                    if (documentHasLists(m_zoneEditors[i].editor->document()))
                        m_zoneEditors[i].zone.richText = extractListHtml(m_zoneEditors[i].editor, m_zoneEditors[i].zone.horizontalAlignment);
                    else
                        m_zoneEditors[i].zone.richText.clear();
                }
                // Recompute vertical position since text height changed
                updateZoneGeometry();
                update();
                emit canvasChanged();
            });

            connect(editor, &QTextEdit::cursorPositionChanged, this, [this, i]() {
                if (i < m_zoneEditors.size() && m_zoneEditors[i].editor->hasFocus()) {
                    if (m_selectedZoneIndex != i) {
                        m_selectedZoneIndex = i;
                        emit zoneSelected(i);
                        update();
                    }
                    emit listStyleChanged(currentListStyle());
                }
            });

            editor->show();

            ZoneEditor ze;
            ze.editor = editor;
            ze.zone = zone;
            m_zoneEditors.append(ze);
        }
        if (!m_zoneEditors.isEmpty())
            m_selectedZoneIndex = 0;
    } else {
        // Blank template: single full-area editor
        QTextEdit* editor = new QTextEdit(this);
        editor->setFrameShape(QFrame::NoFrame);
        editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        editor->setStyleSheet(
            "QTextEdit { background: transparent; border: none; }"
        );
        if (m_slide.hasRichText())
            editor->setHtml(m_slide.richText());
        else
            editor->setPlainText(m_slide.text());
        editor->setPlaceholderText(tr("Enter slide text..."));
        editor->document()->setDocumentMargin(0);

        connect(editor, &QTextEdit::textChanged, this, [this]() {
            updateZoneGeometry();
            update();
            emit canvasChanged();
        });

        connect(editor, &QTextEdit::cursorPositionChanged, this, [this]() {
            emit listStyleChanged(currentListStyle());
        });

        editor->show();

        TextZone blankZone;
        blankZone.id = "text";
        blankZone.text = m_slide.text();
        blankZone.textColor = m_slide.textColor();
        blankZone.fontFamily = m_slide.fontFamily();
        blankZone.fontSize = m_slide.fontSize();
        blankZone.x = 0.05;
        blankZone.y = 0.05;
        blankZone.width = 0.9;
        blankZone.height = 0.9;
        blankZone.horizontalAlignment = 1;
        blankZone.verticalAlignment = 1;

        ZoneEditor ze;
        ze.editor = editor;
        ze.zone = blankZone;
        m_zoneEditors.append(ze);

        m_selectedZoneIndex = 0;
    }

    updateFontScaling();
    updateZoneGeometry();
}

void SlideCanvasWidget::updateZoneGeometry()
{
    // Guard against re-entry: setFrameFormat triggers a document relayout
    // which fires textChanged, which calls updateZoneGeometry again.
    if (m_updatingGeometry)
        return;
    m_updatingGeometry = true;

    QRect sr = slideRect();

    // Before the widget is shown, slideRect() may be empty — skip positioning
    if (sr.width() <= 0 || sr.height() <= 0) {
        m_updatingGeometry = false;
        return;
    }

    for (int i = 0; i < m_zoneEditors.size(); ++i) {
        auto& ze = m_zoneEditors[i];

        // Full zone rect from fractional coordinates
        QRect fullZoneRect(
            sr.x() + qRound(ze.zone.x * sr.width()),
            sr.y() + qRound(ze.zone.y * sr.height()),
            qRound(ze.zone.width * sr.width()),
            qRound(ze.zone.height * sr.height())
        );

        // Always set editor to the full zone rect — never resize based on
        // content height.  Resizing creates a feedback loop where the geometry
        // change alters the document layout, producing a different height on
        // the next measurement and causing visible oscillation.
        ze.editor->setGeometry(fullZoneRect);

        // Vertical alignment is handled via a top margin on the document's
        // root frame.  We must reset the margin to 0 before measuring so
        // that size().height() returns the pure content height — otherwise
        // the previous margin is included, causing an oscillating offset.
        if (fullZoneRect.width() > 0 && fullZoneRect.height() > 0) {
            QTextFrameFormat fmt = ze.editor->document()->rootFrame()->frameFormat();
            fmt.setTopMargin(0);
            ze.editor->document()->rootFrame()->setFrameFormat(fmt);

            ze.editor->document()->setTextWidth(fullZoneRect.width());
            int contentHeight = qRound(ze.editor->document()->size().height());

            int topMargin = 0;
            if (ze.zone.verticalAlignment == 1) {
                topMargin = qMax(0, (fullZoneRect.height() - contentHeight) / 2);
            } else if (ze.zone.verticalAlignment == 2) {
                topMargin = qMax(0, fullZoneRect.height() - contentHeight);
            }

            if (topMargin > 0) {
                fmt.setTopMargin(topMargin);
                ze.editor->document()->rootFrame()->setFrameFormat(fmt);
            }
        }
    }

    m_updatingGeometry = false;
}

void SlideCanvasWidget::updateFontScaling()
{
    double sf = scaleFactor();

    for (int i = 0; i < m_zoneEditors.size(); ++i) {
        auto& ze = m_zoneEditors[i];
        int scaledSize = qMax(8, qRound(ze.zone.fontSize * sf));

        QFont font(ze.zone.fontFamily);
        font.setPixelSize(scaledSize);
        ze.editor->setFont(font);
        ze.editor->setTextColor(ze.zone.textColor);

        // Set the editor's palette text color so list bullet markers
        // (which Qt renders using the palette, not per-character overrides)
        // display in the correct color instead of the default palette color.
        QPalette pal = ze.editor->palette();
        pal.setColor(QPalette::Text, ze.zone.textColor);
        ze.editor->setPalette(pal);

        // Set alignment on all blocks
        Qt::Alignment align = Qt::AlignHCenter;
        if (ze.zone.horizontalAlignment == 0) align = Qt::AlignLeft;
        else if (ze.zone.horizontalAlignment == 2) align = Qt::AlignRight;

        ze.editor->selectAll();
        QTextCursor cursor = ze.editor->textCursor();
        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(align);
        cursor.mergeBlockFormat(blockFormat);

        // Also apply font/color to all existing text — list items have inline
        // char formats that override the editor's default font.
        QTextCharFormat charFormat;
        charFormat.setFont(font);
        charFormat.setForeground(ze.zone.textColor);
        cursor.mergeCharFormat(charFormat);

        cursor.movePosition(QTextCursor::End);
        ze.editor->setTextCursor(cursor);

        // Set the block-level char format on every block so that list bullet
        // markers (which Qt draws using the block's char format, not the
        // text fragment format) render in the correct color.
        {
            QTextCursor bc(ze.editor->document());
            for (QTextBlock block = ze.editor->document()->begin();
                 block != ze.editor->document()->end(); block = block.next()) {
                bc.setPosition(block.position());
                bc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                bc.setBlockCharFormat(charFormat);
            }
        }
    }
}

void SlideCanvasWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Letterbox fill
    painter.fillRect(rect(), QColor(30, 30, 30));

    QRect sr = slideRect();
    Slide currentSlide = buildCurrentSlide();

    // 1. Draw slide background
    switch (currentSlide.backgroundType()) {
    case Slide::Gradient:
        SlidePreviewRenderer::drawGradient(painter, currentSlide, sr);
        break;
    case Slide::Image:
        SlidePreviewRenderer::drawImage(painter, currentSlide, sr);
        break;
    case Slide::Video:
        SlidePreviewRenderer::drawVideo(painter, currentSlide, sr);
        break;
    case Slide::SolidColor:
    default:
        SlidePreviewRenderer::drawBackground(painter, currentSlide, sr);
        break;
    }

    // 2. Draw legibility layers
    if (currentSlide.hasTextZones()) {
        if (currentSlide.overlayEnabled()) {
            painter.fillRect(sr, currentSlide.overlayColor());
        }
        SlidePreviewRenderer::drawTextZoneLegibility(painter, currentSlide, sr);
    } else {
        int margin = qMax(4, sr.width() / 20);
        QRect textAreaRect = sr.adjusted(margin, margin, -margin, -margin);
        QRect textBounds;
        if (!currentSlide.text().isEmpty()) {
            int scaledFontSize = qMax(10, currentSlide.fontSize() * sr.height() / 1080);
            QFont font(currentSlide.fontFamily());
            font.setPixelSize(scaledFontSize);
            QFontMetrics fm(font);
            textBounds = fm.boundingRect(textAreaRect, Qt::AlignCenter | Qt::TextWordWrap,
                                          currentSlide.text());
        }
        SlidePreviewRenderer::drawLegibilityLayers(painter, currentSlide, sr, textBounds);
    }

    // 3. Draw drop shadows by painting each QTextEdit's document at the shadow offset
    //    This guarantees pixel-perfect alignment with the actual text rendering.
    double sf = scaleFactor();

    for (int i = 0; i < m_zoneEditors.size(); ++i) {
        const auto& ze = m_zoneEditors[i];
        bool hasShadow = false;
        QColor shadowColor;
        int shadowOffsetX = 0, shadowOffsetY = 0;

        if (m_slide.slideTemplate() != SlideTemplate::Blank && m_slide.hasTextZones()) {
            hasShadow = ze.zone.dropShadowEnabled;
            shadowColor = ze.zone.dropShadowColor;
            shadowOffsetX = ze.zone.dropShadowOffsetX;
            shadowOffsetY = ze.zone.dropShadowOffsetY;
        } else {
            hasShadow = m_slide.dropShadowEnabled();
            shadowColor = m_slide.dropShadowColor();
            shadowOffsetX = m_slide.dropShadowOffsetX();
            shadowOffsetY = m_slide.dropShadowOffsetY();
        }

        if (!hasShadow || ze.editor->toPlainText().isEmpty())
            continue;

        int scaledOffX = qRound(shadowOffsetX * sf);
        int scaledOffY = qRound(shadowOffsetY * sf);

        // Clone the document and set all text to shadow color
        QTextDocument shadowDoc;
        shadowDoc.setDocumentMargin(0);
        shadowDoc.setDefaultFont(ze.editor->font());
        shadowDoc.setTextWidth(ze.editor->width());

        bool hasLists = documentHasLists(ze.editor->document());
        if (hasLists) {
            // Use HTML to preserve list formatting
            shadowDoc.setHtml(ze.editor->toHtml());
        } else {
            shadowDoc.setPlainText(ze.editor->toPlainText());
        }

        // Apply same alignment as the editor and set shadow color
        QTextCursor cursor(&shadowDoc);
        cursor.select(QTextCursor::Document);
        QTextBlockFormat blockFormat;
        Qt::Alignment align = Qt::AlignHCenter;
        if (ze.zone.horizontalAlignment == 0) align = Qt::AlignLeft;
        else if (ze.zone.horizontalAlignment == 2) align = Qt::AlignRight;
        blockFormat.setAlignment(align);
        cursor.mergeBlockFormat(blockFormat);

        // Set all text to shadow color
        QTextCharFormat charFormat;
        charFormat.setForeground(shadowColor);
        cursor.mergeCharFormat(charFormat);

        // Set block-level char format so list bullet markers use shadow color
        if (hasLists) {
            QTextCursor bc(&shadowDoc);
            for (QTextBlock block = shadowDoc.begin();
                 block != shadowDoc.end(); block = block.next()) {
                bc.setPosition(block.position());
                bc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                bc.setBlockCharFormat(charFormat);
            }
        }

        // Apply same top margin as editor (for vertical alignment)
        double editorTopMargin = ze.editor->document()->rootFrame()->frameFormat().topMargin();
        if (editorTopMargin > 0) {
            QTextFrameFormat shadowFrameFmt = shadowDoc.rootFrame()->frameFormat();
            shadowFrameFmt.setTopMargin(editorTopMargin);
            shadowDoc.rootFrame()->setFrameFormat(shadowFrameFmt);
        }

        // Paint the shadow document at the editor's position + shadow offset
        QPoint editorPos = ze.editor->pos();
        painter.save();
        painter.translate(editorPos.x() + scaledOffX, editorPos.y() + scaledOffY);
        shadowDoc.drawContents(&painter);
        painter.restore();
    }

    // 4. Draw subtle zone selection indicator for multi-zone templates
    if (m_selectedZoneIndex >= 0 && m_selectedZoneIndex < m_zoneEditors.size()
        && m_zoneEditors.size() > 1) {
        const auto& ze = m_zoneEditors[m_selectedZoneIndex];
        QRect zoneRect(
            sr.x() + qRound(ze.zone.x * sr.width()),
            sr.y() + qRound(ze.zone.y * sr.height()),
            qRound(ze.zone.width * sr.width()),
            qRound(ze.zone.height * sr.height())
        );
        QPen pen(QColor(100, 150, 255, 80));
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(zoneRect);
    }
}

void SlideCanvasWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateFontScaling();
    updateZoneGeometry();
}

// --- Per-zone property setters (toolbar -> canvas) ---

void SlideCanvasWidget::setZoneFontFamily(const QString& family)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size())
        return;
    auto& ze = m_zoneEditors[m_selectedZoneIndex];
    ze.zone.fontFamily = family;
    if (m_slide.slideTemplate() == SlideTemplate::Blank)
        m_slide.setFontFamily(family);
    updateFontScaling();
    updateZoneGeometry();
    update();
}

void SlideCanvasWidget::setZoneFontSize(int size)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size())
        return;
    auto& ze = m_zoneEditors[m_selectedZoneIndex];
    ze.zone.fontSize = size;
    if (m_slide.slideTemplate() == SlideTemplate::Blank)
        m_slide.setFontSize(size);
    updateFontScaling();
    updateZoneGeometry();
    update();
}

void SlideCanvasWidget::setZoneTextColor(const QColor& color)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size())
        return;
    auto& ze = m_zoneEditors[m_selectedZoneIndex];
    ze.zone.textColor = color;
    if (m_slide.slideTemplate() == SlideTemplate::Blank)
        m_slide.setTextColor(color);
    updateFontScaling();
    update();
}

void SlideCanvasWidget::setZoneHorizontalAlignment(int align)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size())
        return;
    auto& ze = m_zoneEditors[m_selectedZoneIndex];
    ze.zone.horizontalAlignment = align;
    updateFontScaling();
    update();
}

// --- Slide-level setters ---

void SlideCanvasWidget::setBackgroundColor(const QColor& color)
{
    m_slide.setBackgroundColor(color);
    update();
}

void SlideCanvasWidget::setBackgroundType(Slide::BackgroundType type)
{
    m_slide.setBackgroundType(type);
    update();
}

void SlideCanvasWidget::setGradientStops(const QList<GradientStop>& stops)
{
    m_slide.setGradientStops(stops);
    update();
}

void SlideCanvasWidget::setGradientType(GradientType type)
{
    m_slide.setGradientType(type);
    update();
}

void SlideCanvasWidget::setGradientAngle(int angle)
{
    m_slide.setGradientAngle(angle);
    update();
}

void SlideCanvasWidget::setRadialCenterX(double x)
{
    m_slide.setRadialCenterX(x);
    update();
}

void SlideCanvasWidget::setRadialCenterY(double y)
{
    m_slide.setRadialCenterY(y);
    update();
}

void SlideCanvasWidget::setRadialRadius(double r)
{
    m_slide.setRadialRadius(r);
    update();
}

void SlideCanvasWidget::setBackgroundImagePath(const QString& path)
{
    m_slide.setBackgroundImagePath(path);
    update();
}

void SlideCanvasWidget::setBackgroundImageData(const QByteArray& data)
{
    m_slide.setBackgroundImageData(data);
    update();
}

void SlideCanvasWidget::setBackgroundVideoPath(const QString& path)
{
    m_slide.setBackgroundVideoPath(path);
    update();
}

void SlideCanvasWidget::setVideoLoop(bool loop)
{
    m_slide.setVideoLoop(loop);
}

void SlideCanvasWidget::setOverlayEnabled(bool enabled)
{
    m_slide.setOverlayEnabled(enabled);
    update();
}

void SlideCanvasWidget::setOverlayColor(const QColor& color)
{
    m_slide.setOverlayColor(color);
    update();
}

void SlideCanvasWidget::setOverlayBlur(int blur)
{
    m_slide.setOverlayBlur(blur);
    update();
}

void SlideCanvasWidget::setDropShadowEnabled(bool enabled)
{
    m_slide.setDropShadowEnabled(enabled);
    update();
}

void SlideCanvasWidget::setDropShadowColor(const QColor& color)
{
    m_slide.setDropShadowColor(color);
    update();
}

void SlideCanvasWidget::setDropShadowOffsetX(int offset)
{
    m_slide.setDropShadowOffsetX(offset);
    update();
}

void SlideCanvasWidget::setDropShadowOffsetY(int offset)
{
    m_slide.setDropShadowOffsetY(offset);
    update();
}

void SlideCanvasWidget::setDropShadowBlur(int blur)
{
    m_slide.setDropShadowBlur(blur);
    update();
}

void SlideCanvasWidget::setTextContainerEnabled(bool enabled)
{
    m_slide.setTextContainerEnabled(enabled);
    update();
}

void SlideCanvasWidget::setTextContainerColor(const QColor& color)
{
    m_slide.setTextContainerColor(color);
    update();
}

void SlideCanvasWidget::setTextContainerPadding(int padding)
{
    m_slide.setTextContainerPadding(padding);
    update();
}

void SlideCanvasWidget::setTextContainerRadius(int radius)
{
    m_slide.setTextContainerRadius(radius);
    update();
}

void SlideCanvasWidget::setTextContainerBlur(int blur)
{
    m_slide.setTextContainerBlur(blur);
    update();
}

void SlideCanvasWidget::setTextBandEnabled(bool enabled)
{
    m_slide.setTextBandEnabled(enabled);
    update();
}

void SlideCanvasWidget::setTextBandColor(const QColor& color)
{
    m_slide.setTextBandColor(color);
    update();
}

void SlideCanvasWidget::setTextBandBlur(int blur)
{
    m_slide.setTextBandBlur(blur);
    update();
}

void SlideCanvasWidget::setSlideTemplate(SlideTemplate tmpl)
{
    if (m_slide.slideTemplate() == tmpl)
        return;

    m_slide.setSlideTemplate(tmpl);
    if (tmpl != SlideTemplate::Blank) {
        bool refAtBottom = (tmpl == SlideTemplate::Scripture && m_settings
                            && m_settings->scriptureReferencePosition() == "bottom");
        auto zones = Slide::createTemplateZones(tmpl, refAtBottom);
        m_slide.setTextZones(zones);
    } else {
        m_slide.setTextZones({});
    }
    rebuildZoneEditors();
    update();
    emit canvasChanged();
}

void SlideCanvasWidget::setHasExplicitBackground(bool v)
{
    m_slide.setHasExplicitBackground(v);
}

// --- Per-zone legibility setters ---

void SlideCanvasWidget::setZoneDropShadowEnabled(bool enabled)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.dropShadowEnabled = enabled;
    update();
}

void SlideCanvasWidget::setZoneDropShadowColor(const QColor& color)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.dropShadowColor = color;
    update();
}

void SlideCanvasWidget::setZoneDropShadowOffsetX(int offset)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.dropShadowOffsetX = offset;
    update();
}

void SlideCanvasWidget::setZoneDropShadowOffsetY(int offset)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.dropShadowOffsetY = offset;
    update();
}

void SlideCanvasWidget::setZoneDropShadowBlur(int blur)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.dropShadowBlur = blur;
    update();
}

void SlideCanvasWidget::setZoneTextContainerEnabled(bool enabled)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.textContainerEnabled = enabled;
    update();
}

void SlideCanvasWidget::setZoneTextContainerColor(const QColor& color)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.textContainerColor = color;
    update();
}

void SlideCanvasWidget::setZoneTextContainerPadding(int padding)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.textContainerPadding = padding;
    update();
}

void SlideCanvasWidget::setZoneTextContainerRadius(int radius)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.textContainerRadius = radius;
    update();
}

void SlideCanvasWidget::setZoneTextBandEnabled(bool enabled)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.textBandEnabled = enabled;
    update();
}

void SlideCanvasWidget::setZoneTextBandColor(const QColor& color)
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size()) return;
    m_zoneEditors[m_selectedZoneIndex].zone.textBandColor = color;
    update();
}

// --- List support ---

void SlideCanvasWidget::toggleBulletList()
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size())
        return;
    QTextEdit* editor = m_zoneEditors[m_selectedZoneIndex].editor;
    QTextCursor cursor = editor->textCursor();

    QTextList* currentList = cursor.currentList();
    if (currentList && currentList->format().style() == QTextListFormat::ListDisc) {
        // Remove list formatting from all selected blocks
        QTextBlock block = cursor.block();
        currentList->remove(block);
        QTextBlockFormat bfmt = block.blockFormat();
        bfmt.setIndent(0);
        cursor.setBlockFormat(bfmt);
    } else {
        // If currently in a numbered list, remove it first
        if (currentList) {
            QTextBlock block = cursor.block();
            currentList->remove(block);
            QTextBlockFormat bfmt = block.blockFormat();
            bfmt.setIndent(0);
            cursor.setBlockFormat(bfmt);
        }
        QTextListFormat listFormat;
        listFormat.setStyle(QTextListFormat::ListDisc);
        cursor.createList(listFormat);
    }
    editor->setTextCursor(cursor);

    emit listStyleChanged(currentListStyle());
    update();
    emit canvasChanged();
}

void SlideCanvasWidget::toggleNumberedList()
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size())
        return;
    QTextEdit* editor = m_zoneEditors[m_selectedZoneIndex].editor;
    QTextCursor cursor = editor->textCursor();

    QTextList* currentList = cursor.currentList();
    if (currentList && currentList->format().style() == QTextListFormat::ListDecimal) {
        QTextBlock block = cursor.block();
        currentList->remove(block);
        QTextBlockFormat bfmt = block.blockFormat();
        bfmt.setIndent(0);
        cursor.setBlockFormat(bfmt);
    } else {
        if (currentList) {
            QTextBlock block = cursor.block();
            currentList->remove(block);
            QTextBlockFormat bfmt = block.blockFormat();
            bfmt.setIndent(0);
            cursor.setBlockFormat(bfmt);
        }
        QTextListFormat listFormat;
        listFormat.setStyle(QTextListFormat::ListDecimal);
        cursor.createList(listFormat);
    }
    editor->setTextCursor(cursor);
    emit listStyleChanged(currentListStyle());
    update();
    emit canvasChanged();
}

int SlideCanvasWidget::currentListStyle() const
{
    if (m_selectedZoneIndex < 0 || m_selectedZoneIndex >= m_zoneEditors.size())
        return 0;
    QTextCursor cursor = m_zoneEditors[m_selectedZoneIndex].editor->textCursor();
    QTextList* list = cursor.currentList();
    if (!list)
        return 0;
    QTextListFormat::Style style = list->format().style();
    if (style == QTextListFormat::ListDisc || style == QTextListFormat::ListCircle
        || style == QTextListFormat::ListSquare)
        return 1; // bullet
    if (style == QTextListFormat::ListDecimal || style == QTextListFormat::ListLowerAlpha
        || style == QTextListFormat::ListUpperAlpha)
        return 2; // numbered
    return 0;
}

} // namespace Clarity
