// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "ScriptureInsertDialog.h"
#include "Core/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace Clarity {

ScriptureInsertDialog::ScriptureInsertDialog(BibleDatabase* bible,
                                               EsvApiClient* esvClient,
                                               ApiBibleClient* apiBibleClient,
                                               SettingsManager* settings,
                                               ThemeManager* themeManager,
                                               QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_insertButton(nullptr)
    , m_cancelButton(nullptr)
    , m_localPage(nullptr)
    , m_esvPage(nullptr)
    , m_apiBiblePage(nullptr)
{
    setWindowTitle(tr("Insert Scripture"));
    resize(750, 600);

    setupUI();

    // Create the three tab pages
    m_localPage = new ScriptureDialog(bible, settings, themeManager, this);
    m_esvPage = new EsvScriptureDialog(esvClient, settings, themeManager, this);
    m_apiBiblePage = new ApiBibleScriptureDialog(apiBibleClient, settings, themeManager, this);

    m_tabWidget->addTab(m_localPage, tr("Local Bible"));
    m_tabWidget->addTab(m_esvPage, tr("ESV (API)"));
    m_tabWidget->addTab(m_apiBiblePage, tr("API.bible"));

    // Connect content readiness signals from all pages
    connect(m_localPage, &ScriptureDialog::contentReadyChanged,
            this, &ScriptureInsertDialog::onContentReadyChanged);
    connect(m_esvPage, &EsvScriptureDialog::contentReadyChanged,
            this, &ScriptureInsertDialog::onContentReadyChanged);
    connect(m_apiBiblePage, &ApiBibleScriptureDialog::contentReadyChanged,
            this, &ScriptureInsertDialog::onContentReadyChanged);

    // Update Insert button when switching tabs
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &ScriptureInsertDialog::onTabChanged);

    // Default to Local Bible tab
    m_tabWidget->setCurrentIndex(0);
    m_insertButton->setEnabled(false);
}

void ScriptureInsertDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget, 1);

    // Shared Insert/Cancel buttons at the bottom
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_insertButton = new QPushButton(tr("Insert"), this);
    m_insertButton->setEnabled(false);
    connect(m_insertButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_insertButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

ScriptureInsertDialog::ScriptureSource ScriptureInsertDialog::activeSource() const
{
    switch (m_tabWidget->currentIndex()) {
    case 0: return ScriptureSource::Local;
    case 1: return ScriptureSource::Esv;
    case 2: return ScriptureSource::ApiBible;
    default: return ScriptureSource::Local;
    }
}

void ScriptureInsertDialog::setDefaultStyle(const QColor& bgColor, const QColor& textColor,
                                              const QString& fontFamily, int fontSize)
{
    m_localPage->setDefaultStyle(bgColor, textColor, fontFamily, fontSize);
    m_esvPage->setDefaultStyle(bgColor, textColor, fontFamily, fontSize);
    m_apiBiblePage->setDefaultStyle(bgColor, textColor, fontFamily, fontSize);
}

void ScriptureInsertDialog::onTabChanged(int index)
{
    Q_UNUSED(index)

    // Update Insert button based on current tab's content state
    switch (activeSource()) {
    case ScriptureSource::Local:
        m_insertButton->setEnabled(m_localPage->hasValidContent());
        break;
    case ScriptureSource::Esv:
        m_insertButton->setEnabled(m_esvPage->hasValidContent());
        break;
    case ScriptureSource::ApiBible:
        m_insertButton->setEnabled(m_apiBiblePage->hasValidContent());
        break;
    }
}

void ScriptureInsertDialog::onContentReadyChanged(bool ready)
{
    // Only update if the signal came from the currently active tab
    QWidget* senderWidget = qobject_cast<QWidget*>(sender());
    if (senderWidget == m_tabWidget->currentWidget()) {
        m_insertButton->setEnabled(ready);
    }
}

} // namespace Clarity
