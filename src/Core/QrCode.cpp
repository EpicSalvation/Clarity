// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "QrCode.h"
#include <QPainter>
#include <QDebug>
#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace Clarity {

/*
 * QR Code generator based on the QR Code specification ISO/IEC 18004.
 * This implementation is derived from Project Nayuki's QR Code generator
 * (https://www.nayuki.io/page/qr-code-generator-library), which is released
 * to the public domain.
 *
 * Supports QR Code versions 1-40, all 4 error correction levels, and
 * automatic version/mask selection.
 */

// ---- Forward declarations for internal classes ----

class QrSegmentInternal {
public:
    static QrSegmentInternal makeBytes(const std::vector<uint8_t>& data);

    int mode;  // 0=numeric, 1=alphanumeric, 2=byte, 3=kanji
    int numChars;
    std::vector<bool> data;

    int getTotalBits(int version) const;
};

class QrCodeInternal {
public:
    static QrCodeInternal encodeSegments(const std::vector<QrSegmentInternal>& segs,
                                          int ecl, int minVersion, int maxVersion,
                                          int mask, bool boostEcl);

    int version;
    int size;
    int errorCorrectionLevel;
    int mask;
    std::vector<std::vector<bool>> modules;

private:
    QrCodeInternal(int ver, int ecl, const std::vector<uint8_t>& dataCodewords, int msk);

    void drawFunctionPatterns();
    void drawFormatBits(int msk);
    void drawVersion();
    void drawFinderPattern(int x, int y);
    void drawAlignmentPattern(int x, int y);
    void setFunctionModule(int x, int y, bool isDark);
    bool module(int x, int y) const;

    void drawCodewords(const std::vector<uint8_t>& data);
    void applyMask(int msk);
    long getPenaltyScore() const;

    std::vector<std::vector<bool>> isFunction;

    static std::vector<uint8_t> addEccAndInterleave(const std::vector<uint8_t>& data, int version, int ecl);
    static std::vector<int> getAlignmentPatternPositions(int ver);
    static int getNumRawDataModules(int ver);
    static int getNumDataCodewords(int ver, int ecl);
    static std::vector<uint8_t> reedSolomonComputeDivisor(int degree);
    static std::vector<uint8_t> reedSolomonComputeRemainder(const std::vector<uint8_t>& data,
                                                             const std::vector<uint8_t>& divisor);
    static uint8_t reedSolomonMultiply(uint8_t x, uint8_t y);
    static int finderPenaltyCountPatterns(const std::array<int, 7>& runHistory);
    static int finderPenaltyTerminateAndCount(bool currentRunColor, int currentRunLength,
                                               std::array<int, 7>& runHistory, int qrSize);
    static void finderPenaltyAddHistory(int currentRunLength, std::array<int, 7>& runHistory, int qrSize);

    static constexpr int PENALTY_N1 = 3;
    static constexpr int PENALTY_N2 = 3;
    static constexpr int PENALTY_N3 = 40;
    static constexpr int PENALTY_N4 = 10;

    static const int8_t ECC_CODEWORDS_PER_BLOCK[4][41];
    static const int8_t NUM_ERROR_CORRECTION_BLOCKS[4][41];
};

// ---- QrSegmentInternal implementation ----

QrSegmentInternal QrSegmentInternal::makeBytes(const std::vector<uint8_t>& data) {
    QrSegmentInternal result;
    result.mode = 4;  // Byte mode (0100 binary)
    result.numChars = static_cast<int>(data.size());
    result.data.reserve(data.size() * 8);
    for (uint8_t b : data) {
        for (int i = 7; i >= 0; --i)
            result.data.push_back(((b >> i) & 1) != 0);
    }
    return result;
}

int QrSegmentInternal::getTotalBits(int version) const {
    int ccbits;
    // Mode indicators: Numeric=1, Alphanumeric=2, Byte=4, Kanji=8
    if (mode == 1) ccbits = (version <= 9 ? 10 : (version <= 26 ? 12 : 14));      // Numeric
    else if (mode == 2) ccbits = (version <= 9 ? 9 : (version <= 26 ? 11 : 13));  // Alphanumeric
    else if (mode == 4) ccbits = (version <= 9 ? 8 : 16);                          // Byte
    else ccbits = (version <= 9 ? 8 : (version <= 26 ? 10 : 12));                  // Kanji (8)

    long result = 4L + ccbits + static_cast<long>(data.size());
    if (result > INT_MAX)
        return -1;
    return static_cast<int>(result);
}

// ---- QrCodeInternal implementation ----

const int8_t QrCodeInternal::ECC_CODEWORDS_PER_BLOCK[4][41] = {
    // L
    {-1, 7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28, 28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    // M
    {-1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28},
    // Q
    {-1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30, 28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    // H
    {-1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28, 30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
};

const int8_t QrCodeInternal::NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
    // L
    {-1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 6, 6, 6, 6, 7, 8, 8, 9, 9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},
    // M
    {-1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5, 5, 8, 9, 9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},
    // Q
    {-1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8, 8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},
    // H
    {-1, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81},
};

QrCodeInternal QrCodeInternal::encodeSegments(const std::vector<QrSegmentInternal>& segs,
                                               int ecl, int minVersion, int maxVersion,
                                               int mask, bool boostEcl) {
    if (minVersion < 1 || minVersion > maxVersion || maxVersion > 40)
        throw std::invalid_argument("Invalid version range");

    // Find minimum version
    int version, dataUsedBits;
    for (version = minVersion; ; version++) {
        int dataCapacityBits = getNumDataCodewords(version, ecl) * 8;
        dataUsedBits = 0;
        for (const QrSegmentInternal& seg : segs) {
            int bits = seg.getTotalBits(version);
            if (bits == -1) {
                dataUsedBits = -1;
                break;
            }
            dataUsedBits += bits;
        }
        if (dataUsedBits != -1 && dataUsedBits <= dataCapacityBits)
            break;
        if (version >= maxVersion) {
            throw std::length_error("Data too long");
        }
    }

    // Increase EC level if possible
    if (boostEcl) {
        for (int newEcl = 3; newEcl > ecl; newEcl--) {
            if (dataUsedBits <= getNumDataCodewords(version, newEcl) * 8)
                ecl = newEcl;
        }
    }

    // Build bit stream
    std::vector<bool> bb;
    for (const QrSegmentInternal& seg : segs) {
        // Mode indicator
        int mode = seg.mode;
        bb.push_back((mode >> 3) & 1);
        bb.push_back((mode >> 2) & 1);
        bb.push_back((mode >> 1) & 1);
        bb.push_back((mode >> 0) & 1);

        // Character count - Mode indicators: Numeric=1, Alphanumeric=2, Byte=4, Kanji=8
        int ccbits;
        if (mode == 1) ccbits = (version <= 9 ? 10 : (version <= 26 ? 12 : 14));
        else if (mode == 2) ccbits = (version <= 9 ? 9 : (version <= 26 ? 11 : 13));
        else if (mode == 4) ccbits = (version <= 9 ? 8 : 16);
        else ccbits = (version <= 9 ? 8 : (version <= 26 ? 10 : 12));

        for (int i = ccbits - 1; i >= 0; --i)
            bb.push_back(((seg.numChars >> i) & 1) != 0);

        // Data
        for (bool bit : seg.data)
            bb.push_back(bit);
    }

    // Add terminator and padding
    int dataCapacityBits = getNumDataCodewords(version, ecl) * 8;
    int terminatorBits = std::min(4, dataCapacityBits - static_cast<int>(bb.size()));
    for (int i = 0; i < terminatorBits; ++i)
        bb.push_back(false);

    while (bb.size() % 8 != 0)
        bb.push_back(false);

    // Pad with alternating bytes
    for (uint8_t padByte = 0xEC; bb.size() < static_cast<size_t>(dataCapacityBits); padByte ^= 0xEC ^ 0x11)
        for (int i = 7; i >= 0; --i)
            bb.push_back(((padByte >> i) & 1) != 0);

    // Pack bits into bytes
    std::vector<uint8_t> dataCodewords(bb.size() / 8);
    for (size_t i = 0; i < bb.size(); ++i)
        dataCodewords[i >> 3] |= (bb[i] ? 1 : 0) << (7 - (i & 7));

    return QrCodeInternal(version, ecl, dataCodewords, mask);
}

QrCodeInternal::QrCodeInternal(int ver, int ecl, const std::vector<uint8_t>& dataCodewords, int msk)
    : version(ver)
    , size(ver * 4 + 17)
    , errorCorrectionLevel(ecl)
    , modules(static_cast<size_t>(size), std::vector<bool>(static_cast<size_t>(size)))
    , isFunction(static_cast<size_t>(size), std::vector<bool>(static_cast<size_t>(size)))
{
    drawFunctionPatterns();
    std::vector<uint8_t> allCodewords = addEccAndInterleave(dataCodewords, version, errorCorrectionLevel);
    drawCodewords(allCodewords);

    if (msk == -1) {
        long minPenalty = LONG_MAX;
        for (int i = 0; i < 8; ++i) {
            applyMask(i);
            drawFormatBits(i);
            long penalty = getPenaltyScore();
            if (penalty < minPenalty) {
                msk = i;
                minPenalty = penalty;
            }
            applyMask(i);  // Undo
        }
    }
    mask = msk;
    applyMask(msk);
    drawFormatBits(msk);
    isFunction.clear();
}

void QrCodeInternal::drawFunctionPatterns() {
    // Draw horizontal and vertical timing patterns
    for (int i = 0; i < size; ++i) {
        setFunctionModule(6, i, i % 2 == 0);
        setFunctionModule(i, 6, i % 2 == 0);
    }

    // Draw 3 finder patterns
    drawFinderPattern(3, 3);
    drawFinderPattern(size - 4, 3);
    drawFinderPattern(3, size - 4);

    // Draw alignment patterns
    std::vector<int> alignPatPos = getAlignmentPatternPositions(version);
    size_t numAlign = alignPatPos.size();
    for (size_t i = 0; i < numAlign; ++i) {
        for (size_t j = 0; j < numAlign; ++j) {
            if (!((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0)))
                drawAlignmentPattern(alignPatPos[i], alignPatPos[j]);
        }
    }

    // Draw format bits (will be overwritten)
    drawFormatBits(0);
    drawVersion();
}

void QrCodeInternal::drawFormatBits(int msk) {
    // Calculate format bits
    // EC level encoding: L=1, M=0, Q=3, H=2 (not sequential!)
    static const int EC_FORMAT_BITS[] = {1, 0, 3, 2};  // Maps our 0-3 (L,M,Q,H) to spec encoding
    int data = EC_FORMAT_BITS[errorCorrectionLevel] << 3 | msk;
    int rem = data;
    for (int i = 0; i < 10; ++i)
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    int bits = (data << 10 | rem) ^ 0x5412;

    // Draw first copy
    for (int i = 0; i <= 5; ++i)
        setFunctionModule(8, i, ((bits >> i) & 1) != 0);
    setFunctionModule(8, 7, ((bits >> 6) & 1) != 0);
    setFunctionModule(8, 8, ((bits >> 7) & 1) != 0);
    setFunctionModule(7, 8, ((bits >> 8) & 1) != 0);
    for (int i = 9; i < 15; ++i)
        setFunctionModule(14 - i, 8, ((bits >> i) & 1) != 0);

    // Draw second copy
    for (int i = 0; i < 8; ++i)
        setFunctionModule(size - 1 - i, 8, ((bits >> i) & 1) != 0);
    for (int i = 8; i < 15; ++i)
        setFunctionModule(8, size - 15 + i, ((bits >> i) & 1) != 0);
    setFunctionModule(8, size - 8, true);  // Always dark
}

void QrCodeInternal::drawVersion() {
    if (version < 7)
        return;

    int rem = version;
    for (int i = 0; i < 12; ++i)
        rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
    long bits = static_cast<long>(version) << 12 | rem;

    for (int i = 0; i < 18; ++i) {
        bool bit = ((bits >> i) & 1) != 0;
        int a = size - 11 + i % 3;
        int b = i / 3;
        setFunctionModule(a, b, bit);
        setFunctionModule(b, a, bit);
    }
}

void QrCodeInternal::drawFinderPattern(int x, int y) {
    for (int dy = -4; dy <= 4; ++dy) {
        for (int dx = -4; dx <= 4; ++dx) {
            int dist = std::max(std::abs(dx), std::abs(dy));
            int xx = x + dx, yy = y + dy;
            if (0 <= xx && xx < size && 0 <= yy && yy < size)
                setFunctionModule(xx, yy, dist != 2 && dist != 4);
        }
    }
}

void QrCodeInternal::drawAlignmentPattern(int x, int y) {
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx)
            setFunctionModule(x + dx, y + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
    }
}

void QrCodeInternal::setFunctionModule(int x, int y, bool isDark) {
    modules[static_cast<size_t>(y)][static_cast<size_t>(x)] = isDark;
    isFunction[static_cast<size_t>(y)][static_cast<size_t>(x)] = true;
}

bool QrCodeInternal::module(int x, int y) const {
    return modules[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

void QrCodeInternal::drawCodewords(const std::vector<uint8_t>& data) {
    size_t i = 0;
    for (int right = size - 1; right >= 1; right -= 2) {
        if (right == 6)
            right = 5;
        for (int vert = 0; vert < size; ++vert) {
            for (int j = 0; j < 2; ++j) {
                int x = right - j;
                bool upward = ((right + 1) & 2) == 0;
                int y = upward ? size - 1 - vert : vert;
                if (!isFunction[static_cast<size_t>(y)][static_cast<size_t>(x)] && i < data.size() * 8) {
                    modules[static_cast<size_t>(y)][static_cast<size_t>(x)] =
                        ((data[i >> 3] >> (7 - (i & 7))) & 1) != 0;
                    ++i;
                }
            }
        }
    }
}

void QrCodeInternal::applyMask(int msk) {
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool invert;
            switch (msk) {
                case 0: invert = (x + y) % 2 == 0; break;
                case 1: invert = y % 2 == 0; break;
                case 2: invert = x % 3 == 0; break;
                case 3: invert = (x + y) % 3 == 0; break;
                case 4: invert = (x / 3 + y / 2) % 2 == 0; break;
                case 5: invert = x * y % 2 + x * y % 3 == 0; break;
                case 6: invert = (x * y % 2 + x * y % 3) % 2 == 0; break;
                case 7: invert = ((x + y) % 2 + x * y % 3) % 2 == 0; break;
                default: throw std::logic_error("Invalid mask");
            }
            bool isFuncModule = isFunction[static_cast<size_t>(y)][static_cast<size_t>(x)];
            if (invert && !isFuncModule) {
                bool current = modules[static_cast<size_t>(y)][static_cast<size_t>(x)];
                modules[static_cast<size_t>(y)][static_cast<size_t>(x)] = !current;
            }
        }
    }
}

long QrCodeInternal::getPenaltyScore() const {
    long result = 0;

    // Adjacent modules in row/column
    for (int y = 0; y < size; ++y) {
        bool runColor = false;
        int runX = 0;
        std::array<int, 7> runHistory = {};
        for (int x = 0; x < size; ++x) {
            if (module(x, y) == runColor) {
                runX++;
                if (runX == 5)
                    result += PENALTY_N1;
                else if (runX > 5)
                    result++;
            } else {
                finderPenaltyAddHistory(runX, runHistory, size);
                if (!runColor)
                    result += finderPenaltyCountPatterns(runHistory) * PENALTY_N3;
                runColor = module(x, y);
                runX = 1;
            }
        }
        result += finderPenaltyTerminateAndCount(runColor, runX, runHistory, size) * PENALTY_N3;
    }

    for (int x = 0; x < size; ++x) {
        bool runColor = false;
        int runY = 0;
        std::array<int, 7> runHistory = {};
        for (int y = 0; y < size; ++y) {
            if (module(x, y) == runColor) {
                runY++;
                if (runY == 5)
                    result += PENALTY_N1;
                else if (runY > 5)
                    result++;
            } else {
                finderPenaltyAddHistory(runY, runHistory, size);
                if (!runColor)
                    result += finderPenaltyCountPatterns(runHistory) * PENALTY_N3;
                runColor = module(x, y);
                runY = 1;
            }
        }
        result += finderPenaltyTerminateAndCount(runColor, runY, runHistory, size) * PENALTY_N3;
    }

    // 2x2 blocks
    for (int y = 0; y < size - 1; ++y) {
        for (int x = 0; x < size - 1; ++x) {
            bool color = module(x, y);
            if (color == module(x + 1, y) && color == module(x, y + 1) && color == module(x + 1, y + 1))
                result += PENALTY_N2;
        }
    }

    // Balance of dark/light
    int dark = 0;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (module(x, y))
                dark++;
        }
    }
    int total = size * size;
    int k = static_cast<int>((std::abs(dark * 20L - total * 10L) + total - 1) / total) - 1;
    result += k * PENALTY_N4;

    return result;
}

std::vector<uint8_t> QrCodeInternal::addEccAndInterleave(const std::vector<uint8_t>& data,
                                                          int version, int ecl) {
    int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[ecl][version];
    int blockEccLen = ECC_CODEWORDS_PER_BLOCK[ecl][version];
    int rawCodewords = getNumRawDataModules(version) / 8;
    int numShortBlocks = numBlocks - rawCodewords % numBlocks;
    int shortBlockLen = rawCodewords / numBlocks;

    std::vector<std::vector<uint8_t>> blocks;
    std::vector<uint8_t> rsDiv = reedSolomonComputeDivisor(blockEccLen);
    for (int i = 0, k = 0; i < numBlocks; ++i) {
        std::vector<uint8_t> dat(data.cbegin() + k, data.cbegin() + (k + shortBlockLen - blockEccLen + (i < numShortBlocks ? 0 : 1)));
        k += static_cast<int>(dat.size());
        std::vector<uint8_t> ecc = reedSolomonComputeRemainder(dat, rsDiv);
        if (i < numShortBlocks)
            dat.push_back(0);
        dat.insert(dat.end(), ecc.cbegin(), ecc.cend());
        blocks.push_back(std::move(dat));
    }

    std::vector<uint8_t> result;
    for (size_t i = 0; i < blocks[0].size(); ++i) {
        for (size_t j = 0; j < blocks.size(); ++j) {
            if (i != static_cast<size_t>(shortBlockLen - blockEccLen) || j >= static_cast<size_t>(numShortBlocks))
                result.push_back(blocks[j][i]);
        }
    }
    return result;
}

std::vector<int> QrCodeInternal::getAlignmentPatternPositions(int ver) {
    if (ver == 1)
        return {};
    int numAlign = ver / 7 + 2;
    int step = (ver == 32) ? 26 : (ver * 4 + numAlign * 2 + 1) / (numAlign * 2 - 2) * 2;
    std::vector<int> result;
    for (int i = 0, pos = ver * 4 + 10; i < numAlign - 1; ++i, pos -= step)
        result.insert(result.begin(), pos);
    result.insert(result.begin(), 6);
    return result;
}

int QrCodeInternal::getNumRawDataModules(int ver) {
    int result = (16 * ver + 128) * ver + 64;
    if (ver >= 2) {
        int numAlign = ver / 7 + 2;
        result -= (25 * numAlign - 10) * numAlign - 55;
        if (ver >= 7)
            result -= 36;
    }
    return result;
}

int QrCodeInternal::getNumDataCodewords(int ver, int ecl) {
    return getNumRawDataModules(ver) / 8
        - ECC_CODEWORDS_PER_BLOCK[ecl][ver] * NUM_ERROR_CORRECTION_BLOCKS[ecl][ver];
}

std::vector<uint8_t> QrCodeInternal::reedSolomonComputeDivisor(int degree) {
    std::vector<uint8_t> result(static_cast<size_t>(degree));
    result[static_cast<size_t>(degree) - 1] = 1;
    uint8_t root = 1;
    for (int i = 0; i < degree; ++i) {
        for (size_t j = 0; j < result.size(); ++j) {
            result[j] = reedSolomonMultiply(result[j], root);
            if (j + 1 < result.size())
                result[j] ^= result[j + 1];
        }
        root = reedSolomonMultiply(root, 0x02);
    }
    return result;
}

std::vector<uint8_t> QrCodeInternal::reedSolomonComputeRemainder(const std::vector<uint8_t>& data,
                                                                  const std::vector<uint8_t>& divisor) {
    std::vector<uint8_t> result(divisor.size());
    for (uint8_t b : data) {
        uint8_t factor = b ^ result[0];
        result.erase(result.begin());
        result.push_back(0);
        for (size_t i = 0; i < result.size(); ++i)
            result[i] ^= reedSolomonMultiply(divisor[i], factor);
    }
    return result;
}

uint8_t QrCodeInternal::reedSolomonMultiply(uint8_t x, uint8_t y) {
    int z = 0;
    for (int i = 7; i >= 0; --i) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return static_cast<uint8_t>(z);
}

int QrCodeInternal::finderPenaltyCountPatterns(const std::array<int, 7>& runHistory) {
    int n = runHistory[1];
    bool core = n > 0 && runHistory[2] == n && runHistory[3] == n * 3 && runHistory[4] == n && runHistory[5] == n;
    return (core && runHistory[0] >= n * 4 && runHistory[6] >= n ? 1 : 0)
         + (core && runHistory[6] >= n * 4 && runHistory[0] >= n ? 1 : 0);
}

int QrCodeInternal::finderPenaltyTerminateAndCount(bool currentRunColor, int currentRunLength,
                                                    std::array<int, 7>& runHistory, int qrSize) {
    if (currentRunColor) {
        finderPenaltyAddHistory(currentRunLength, runHistory, qrSize);
        currentRunLength = 0;
    }
    currentRunLength += qrSize;
    finderPenaltyAddHistory(currentRunLength, runHistory, qrSize);
    return finderPenaltyCountPatterns(runHistory);
}

void QrCodeInternal::finderPenaltyAddHistory(int currentRunLength, std::array<int, 7>& runHistory, int qrSize) {
    if (runHistory[0] == 0)
        currentRunLength += qrSize;
    std::copy_backward(runHistory.cbegin(), runHistory.cend() - 1, runHistory.end());
    runHistory[0] = currentRunLength;
}

// ---- Public QrCode API ----

QImage QrCode::generate(const QString& text, int moduleSize, int margin,
                        const QColor& foreground, const QColor& background) {
    auto matrix = generateMatrix(text);
    if (matrix.empty()) {
        return QImage();
    }

    int size = static_cast<int>(matrix.size());
    int imageSize = (size + margin * 2) * moduleSize;

    QImage image(imageSize, imageSize, QImage::Format_RGB32);
    image.fill(background);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(foreground);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (matrix[static_cast<size_t>(y)][static_cast<size_t>(x)]) {
                painter.drawRect((x + margin) * moduleSize,
                                 (y + margin) * moduleSize,
                                 moduleSize, moduleSize);
            }
        }
    }

    return image;
}

std::vector<std::vector<bool>> QrCode::generateMatrix(const QString& text) {
    QByteArray data = text.toUtf8();

    if (data.isEmpty()) {
        qWarning() << "QrCode: Empty input";
        return {};
    }

    try {
        // Convert to std::vector
        std::vector<uint8_t> bytes(data.begin(), data.end());

        // Create byte segment
        QrSegmentInternal seg = QrSegmentInternal::makeBytes(bytes);

        // Encode with EC level M, auto version selection, auto mask
        QrCodeInternal qr = QrCodeInternal::encodeSegments({seg}, 1, 1, 40, -1, true);

        return qr.modules;
    } catch (const std::exception& e) {
        qWarning() << "QrCode: Failed to generate:" << e.what();
        return {};
    }
}

} // namespace Clarity
