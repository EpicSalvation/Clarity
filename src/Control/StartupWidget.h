#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

namespace Clarity {

class StartupWidget : public QWidget {
    Q_OBJECT

public:
    explicit StartupWidget(QWidget* parent = nullptr);

    void setRecentFiles(const QStringList& files);

signals:
    void newPresentationRequested();
    void openPresentationRequested();
    void openRecentRequested(const QString& filePath);

private:
    QLabel* m_recentLabel;
    QListWidget* m_recentList;
    QWidget* m_recentWidget;
};

} // namespace Clarity
