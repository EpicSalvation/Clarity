// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Troy Dontigney

#include "UndoManager.h"

namespace Clarity {

UndoManager::UndoManager(QObject* parent)
    : QObject(parent)
{
}

void UndoManager::pushSnapshot(const QJsonObject& snapshot, const QString& description)
{
    bool couldUndo = canUndo();
    bool couldRedo = canRedo();

    m_undoStack.append({snapshot, description});

    // Trim oldest if over limit
    while (m_undoStack.size() > MaxUndoLevels) {
        m_undoStack.removeFirst();
    }

    // New action invalidates redo history
    m_redoStack.clear();

    if (!couldUndo || couldRedo) {
        emit undoRedoStateChanged();
    }
}

QJsonObject UndoManager::undo(const QJsonObject& currentState)
{
    if (m_undoStack.isEmpty()) {
        return {};
    }

    Snapshot previous = m_undoStack.takeLast();

    // Push current state onto redo stack with the same description
    m_redoStack.append({currentState, previous.description});

    emit undoRedoStateChanged();
    return previous.data;
}

QJsonObject UndoManager::redo(const QJsonObject& currentState)
{
    if (m_redoStack.isEmpty()) {
        return {};
    }

    Snapshot next = m_redoStack.takeLast();

    // Push current state onto undo stack with the same description
    m_undoStack.append({currentState, next.description});

    emit undoRedoStateChanged();
    return next.data;
}

void UndoManager::clear()
{
    bool hadState = canUndo() || canRedo();
    m_undoStack.clear();
    m_redoStack.clear();
    if (hadState) {
        emit undoRedoStateChanged();
    }
}

bool UndoManager::canUndo() const
{
    return !m_undoStack.isEmpty();
}

bool UndoManager::canRedo() const
{
    return !m_redoStack.isEmpty();
}

QString UndoManager::undoDescription() const
{
    if (m_undoStack.isEmpty()) return {};
    return m_undoStack.last().description;
}

QString UndoManager::redoDescription() const
{
    if (m_redoStack.isEmpty()) return {};
    return m_redoStack.last().description;
}

} // namespace Clarity
