// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>

class QDialog;
class QKeySequence;
class QSettings;
class QShortcut;

struct Hotkey {
    Hotkey() : shortcut(nullptr), context(Qt::WindowShortcut) {}

    QKeySequence keyseq;
    QShortcut* shortcut;
    Qt::ShortcutContext context;
};

typedef std::map<QString, Hotkey> HotkeyMap;
typedef std::map<QString, HotkeyMap> HotkeyGroupMap;

extern HotkeyGroupMap hotkey_groups;

/**
 * Returns a QShortcut object whose activated() signal can be connected to other QObjects' slots.
 *
 * @param group  General group this hotkey belongs to (e.g. "Main Window", "Debugger").
 * @param action Name of the action (e.g. "Start Emulation", "Load Image").
 * @param widget Parent widget of the returned QShortcut.
 * @warning If multiple QWidgets' call this function for the same action, the returned QShortcut
 * will be the same. Thus, you shouldn't rely on the caller really being the QShortcut's parent.
 */
QShortcut* GetHotkey(const QString& group, const QString& action, QWidget* widget);

/**
* Returns a QKeySequence object who signal can be connected to QAction->SetShortcut.
*
* @param group  General group this hotkey belongs to (e.g. "Main Window", "Debugger").
* @param action Name of the action (e.g. "Start Emulation", "Load Image").
*/
QKeySequence GetKeySequence(const QString& group, const QString& action);

/**
* Returns a Qt::ShortcutContext object who can be connected to other QAction->SetShortcutContext.
*
* @param group  General group this shortcutcontext belongs to (e.g. "Main Window", "Debugger").
* @param action Name of the action (e.g. "Start Emulation", "Load Image").
*/
Qt::ShortcutContext GetShortcutContext(const QString& group, const QString& action);

/**
 * Saves all registered hotkeys to the settings file.
 *
 * @note Each hotkey group will be stored a settings group; For each hotkey inside that group, a
 * settings group will be created to store the key sequence and the hotkey context.
 */
void SaveHotkeys();

/**
 * Loads hotkeys from the settings file.
 *
 * @note Yet unregistered hotkeys which are present in the settings will automatically be
 * registered.
 */
void LoadHotkeys();
