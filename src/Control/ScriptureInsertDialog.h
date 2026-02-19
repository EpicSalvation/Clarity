#pragma once

#include "ScriptureDialog.h"
#include "EsvScriptureDialog.h"
#include "ApiBibleScriptureDialog.h"
#include <QDialog>
#include <QTabWidget>
#include <QPushButton>

namespace Clarity {

class BibleDatabase;
class EsvApiClient;
class ApiBibleClient;
class SettingsManager;
class ThemeManager;

/**
 * @brief Tabbed dialog wrapping all scripture sources (Local Bible, ESV API, API.bible)
 *
 * Presents each scripture source as a tab. The Insert button is shared across tabs
 * and enables/disables based on the active tab's content readiness.
 */
class ScriptureInsertDialog : public QDialog {
    Q_OBJECT

public:
    enum class ScriptureSource { Local, Esv, ApiBible };

    explicit ScriptureInsertDialog(BibleDatabase* bible,
                                    EsvApiClient* esvClient,
                                    ApiBibleClient* apiBibleClient,
                                    SettingsManager* settings,
                                    ThemeManager* themeManager,
                                    QWidget* parent = nullptr);

    /**
     * @brief Which scripture source tab is currently active
     */
    ScriptureSource activeSource() const;

    /**
     * @brief Set default slide style for all three tabs
     */
    void setDefaultStyle(const QColor& bgColor, const QColor& textColor,
                         const QString& fontFamily, int fontSize);

    // Accessors that delegate to the active tab
    ScriptureDialog* localPage() const { return m_localPage; }
    EsvScriptureDialog* esvPage() const { return m_esvPage; }
    ApiBibleScriptureDialog* apiBiblePage() const { return m_apiBiblePage; }

private slots:
    void onTabChanged(int index);
    void onContentReadyChanged(bool ready);

private:
    void setupUI();

    QTabWidget* m_tabWidget;
    QPushButton* m_insertButton;
    QPushButton* m_cancelButton;

    ScriptureDialog* m_localPage;
    EsvScriptureDialog* m_esvPage;
    ApiBibleScriptureDialog* m_apiBiblePage;
};

} // namespace Clarity
