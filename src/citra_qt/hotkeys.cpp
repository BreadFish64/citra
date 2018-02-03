// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QKeySequence>
#include <QShortcut>
#include <QtGlobal>
#include "citra_qt/hotkeys.h"
#include "citra_qt/ui_settings.h"

HotkeyGroupMap hotkey_groups = {};

void SaveHotkeys() {
    UISettings::values.shortcuts.clear();
    for (auto group : hotkey_groups) {
        for (auto hotkey : group.second) {
            UISettings::values.shortcuts.emplace_back(
                UISettings::Shortcut(group.first + "/" + hotkey.first,
                                     UISettings::ContextualShortcut(hotkey.second.keyseq.toString(),
                                                                    hotkey.second.context)));
        }
    }
}

void LoadHotkeys() {
    // Make sure NOT to use a reference here because it would become invalid once we call
    // beginGroup()
    for (auto shortcut : UISettings::values.shortcuts) {
        QStringList cat = shortcut.first.split("/");
        Q_ASSERT(cat.size() >= 2);

        // RegisterHotkey assigns default keybindings, so use old values as default parameters
        Hotkey& hk = hotkey_groups[cat[0]][cat[1]];
        if (!shortcut.second.first.isEmpty()) {
            hk.keyseq = QKeySequence::fromString(shortcut.second.first);
            hk.context = (Qt::ShortcutContext)shortcut.second.second;
        }
        if (hk.shortcut)
            hk.shortcut->setKey(hk.keyseq);
    }
}

QShortcut* GetHotkey(const QString& group, const QString& action, QWidget* widget) {
    Hotkey& hk = hotkey_groups[group][action];

    if (!hk.shortcut)
        hk.shortcut = new QShortcut(hk.keyseq, widget, nullptr, nullptr, hk.context);

    return hk.shortcut;
}

QKeySequence GetKeySequence(const QString& group, const QString& action) {
    Hotkey& hk = hotkey_groups[group][action];
    return hk.keyseq;
}

Qt::ShortcutContext GetShortcutContext(const QString& group, const QString& action) {
    Hotkey& hk = hotkey_groups[group][action];
    return hk.context;
}
