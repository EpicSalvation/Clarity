#pragma once

#include <QImage>
#include <QString>
#include <QColor>
#include <vector>

namespace Clarity {

/**
 * @brief QR code generator
 *
 * Generates QR codes from text data. Based on Project Nayuki's QR Code generator,
 * which is released to the public domain.
 *
 * Supports QR Code versions 1-40, error correction level M, and automatic
 * version/mask selection for optimal readability.
 */
class QrCode {
public:
    /**
     * @brief Generate a QR code image from text
     * @param text The text to encode (typically a URL)
     * @param moduleSize Size of each module (pixel) in the output image
     * @param margin Quiet zone margin in modules (minimum 4 recommended)
     * @param foreground Foreground (dark module) color
     * @param background Background (light module) color
     * @return QImage containing the QR code, or null image on failure
     */
    static QImage generate(const QString& text,
                           int moduleSize = 8,
                           int margin = 4,
                           const QColor& foreground = Qt::black,
                           const QColor& background = Qt::white);

    /**
     * @brief Generate a QR code as a 2D boolean matrix
     * @param text The text to encode
     * @return Vector of rows, each row is a vector of bools (true = dark module)
     */
    static std::vector<std::vector<bool>> generateMatrix(const QString& text);
};

} // namespace Clarity
