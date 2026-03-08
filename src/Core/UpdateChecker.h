// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Clarity {

/**
 * @brief Async GitHub releases checker
 *
 * Queries the GitHub releases/latest API and compares the tag against the
 * running CLARITY_VERSION. Create on demand, parent to the caller so it is
 * automatically destroyed; connect signals before calling check().
 *
 * Usage:
 *   auto* checker = new UpdateChecker(this);
 *   connect(checker, &UpdateChecker::updateAvailable, ...);
 *   checker->check();
 */
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);

    /**
     * @brief Initiates the HTTP request to GitHub. Call once after connecting signals.
     * @param includeBeta When true, pre-release builds are considered as valid updates.
     */
    void check(bool includeBeta = false);

signals:
    /** Emitted when a newer version is available on GitHub. */
    void updateAvailable(const QString& latestVersion, const QUrl& releaseUrl);

    /** Emitted when the running version is already up to date. */
    void upToDate();

    /** Emitted when the check could not be completed (network error, parse error, etc.). */
    void checkFailed(const QString& error);

private slots:
    void onReply(QNetworkReply* reply);

private:
    /**
     * @brief Compares two version strings.
     * @return -1 if a < b, 0 if a == b, 1 if a > b
     *
     * Handles semver with optional pre-release suffix separated by '-':
     *   "1.2.3" > "1.2.3-beta.1"  (release > pre-release)
     *   "1.2.3-beta.2" > "1.2.3-beta.1"  (string comparison on suffix)
     */
    static int compareVersions(const QString& a, const QString& b);

    QNetworkAccessManager* m_network;
    bool m_includeBeta = false;
};

} // namespace Clarity
