// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QList>

namespace Clarity {

/**
 * @brief Snapshot-based undo/redo manager for presentation edits
 *
 * Stores full Presentation::toJson() snapshots before each mutation.
 * On undo, restores the previous snapshot via Presentation::fromJson().
 * Max 50 undo levels; oldest entries are trimmed on overflow.
 */
class UndoManager : public QObject {
    Q_OBJECT

public:
    explicit UndoManager(QObject* parent = nullptr);

    /**
     * @brief Save a snapshot before a mutation
     * @param snapshot The current presentation state (Presentation::toJson())
     * @param description Human-readable description (e.g., "Edit Slide")
     *
     * Clears the redo stack (new action invalidates redo history).
     */
    void pushSnapshot(const QJsonObject& snapshot, const QString& description);

    /**
     * @brief Undo the last action
     * @param currentState The current presentation state to push onto redo stack
     * @return The previous state to restore, or empty QJsonObject if nothing to undo
     */
    QJsonObject undo(const QJsonObject& currentState);

    /**
     * @brief Redo the last undone action
     * @param currentState The current presentation state to push onto undo stack
     * @return The next state to restore, or empty QJsonObject if nothing to redo
     */
    QJsonObject redo(const QJsonObject& currentState);

    /** @brief Clear both undo and redo stacks (e.g., on new/open) */
    void clear();

    bool canUndo() const;
    bool canRedo() const;

    /** @brief Description of the action that would be undone */
    QString undoDescription() const;

    /** @brief Description of the action that would be redone */
    QString redoDescription() const;

signals:
    /** @brief Emitted when canUndo/canRedo state changes */
    void undoRedoStateChanged();

private:
    struct Snapshot {
        QJsonObject data;
        QString description;
    };

    static constexpr int MaxUndoLevels = 50;

    QList<Snapshot> m_undoStack;
    QList<Snapshot> m_redoStack;
};

} // namespace Clarity
