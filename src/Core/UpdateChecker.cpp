// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "UpdateChecker.h"
#include "Core/Version.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QDebug>

namespace Clarity {

static const QUrl GITHUB_RELEASES_URL{
    "https://api.github.com/repos/EpicSalvation/Clarity/releases?per_page=20"};

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void UpdateChecker::check(bool includeBeta)
{
    m_includeBeta = includeBeta;

    QNetworkRequest request(GITHUB_RELEASES_URL);
    // GitHub API requires a User-Agent header
    request.setRawHeader("User-Agent", QByteArray("Clarity/") + CLARITY_VERSION);
    request.setRawHeader("Accept", "application/vnd.github+json");

    connect(m_network, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onReply);

    m_network->get(request);
    qDebug() << "UpdateChecker: Checking for updates (includeBeta=" << includeBeta << ")";
}

void UpdateChecker::onReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "UpdateChecker: Network error:" << reply->errorString();
        emit checkFailed(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "UpdateChecker: JSON parse error:" << parseError.errorString();
        emit checkFailed(tr("Invalid response from server."));
        return;
    }

    if (!doc.isArray()) {
        qWarning() << "UpdateChecker: Expected a JSON array from releases endpoint";
        emit checkFailed(tr("Unexpected response format from server."));
        return;
    }

    // Find the highest-versioned release that matches the channel.
    // GitHub returns releases newest-first, but we compare versions explicitly
    // so ordering doesn't matter.
    QString bestVersion;
    QString bestUrl;

    for (const QJsonValue& val : doc.array()) {
        QJsonObject obj = val.toObject();
        bool isPrerelease = obj.value("prerelease").toBool();
        if (isPrerelease && !m_includeBeta)
            continue;

        QString tag = obj.value("tag_name").toString();
        QString url = obj.value("html_url").toString();
        if (tag.isEmpty() || url.isEmpty())
            continue;

        // Strip leading 'v'
        if (tag.startsWith('v') || tag.startsWith('V'))
            tag.remove(0, 1);

        if (bestVersion.isEmpty() || compareVersions(tag, bestVersion) > 0) {
            bestVersion = tag;
            bestUrl = url;
        }
    }

    if (bestVersion.isEmpty()) {
        qDebug() << "UpdateChecker: No eligible releases found";
        emit upToDate();
        return;
    }

    QString currentVersion = QString(CLARITY_VERSION);
    qDebug() << "UpdateChecker: current=" << currentVersion << "best candidate=" << bestVersion;

    if (compareVersions(bestVersion, currentVersion) > 0) {
        emit updateAvailable(bestVersion, QUrl(bestUrl));
    } else {
        emit upToDate();
    }
}

int UpdateChecker::compareVersions(const QString& a, const QString& b)
{
    // Split "X.Y.Z-suffix" into base and suffix parts
    auto split = [](const QString& ver, QString& base, QString& suffix) {
        int dash = ver.indexOf('-');
        if (dash >= 0) {
            base = ver.left(dash);
            suffix = ver.mid(dash + 1);
        } else {
            base = ver;
            suffix.clear();
        }
    };

    QString baseA, suffixA, baseB, suffixB;
    split(a, baseA, suffixA);
    split(b, baseB, suffixB);

    // Compare numeric version parts
    QStringList partsA = baseA.split('.');
    QStringList partsB = baseB.split('.');
    int maxLen = qMax(partsA.size(), partsB.size());
    for (int i = 0; i < maxLen; ++i) {
        int numA = (i < partsA.size()) ? partsA[i].toInt() : 0;
        int numB = (i < partsB.size()) ? partsB[i].toInt() : 0;
        if (numA < numB) return -1;
        if (numA > numB) return  1;
    }

    // Bases are equal — release > pre-release, pre-release compared as strings
    if (suffixA.isEmpty() && suffixB.isEmpty()) return 0;
    if (suffixA.isEmpty()) return  1;  // a has no suffix → a is a release → a > b
    if (suffixB.isEmpty()) return -1;  // b has no suffix → b is a release → b > a
    if (suffixA < suffixB) return -1;
    if (suffixA > suffixB) return  1;
    return 0;
}

} // namespace Clarity
