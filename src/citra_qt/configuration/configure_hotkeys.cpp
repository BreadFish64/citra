// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QKeyEvent>
#include <QTimer>
#include "citra_qt/configuration/configure_hotkeys.h"
#include "citra_qt/hotkeys.h"
#include "citra_qt/ui_settings.h"
#include "common/param_package.h"
#include "core/settings.h"
#include "input_common/main.h"
#include "ui_configure_hotkeys.h"

ConfigureHotkeys::ConfigureHotkeys(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureHotkeys>()),
      timer(std::make_unique<QTimer>()) {

    ui->setupUi(this);
    setFocusPolicy(Qt::ClickFocus);

    QStandardItemModel* model = new QStandardItemModel(this);
    model->setColumnCount(3);
    model->setHorizontalHeaderLabels({tr("Action"), tr("Hotkey"), tr("Context")});

    for (auto group : hotkey_groups) {
        QStandardItem* parent_item = new QStandardItem(group.first);
        parent_item->setEditable(false);
        for (auto hotkey : group.second) {
            QStandardItem* action = new QStandardItem(hotkey.first);
            QStandardItem* keyseq = new QStandardItem(hotkey.second.keyseq.toString());
            if (keyseq->text() == "") {
                keyseq->setText("-");
            }
            action->setEditable(false);
            keyseq->setEditable(false);
            parent_item->appendRow({action, keyseq});
        }
        model->appendRow(parent_item);
    }

    ui->treeView->setSelectionMode(QTreeView::SingleSelection);
    ui->treeView->setModel(model);

    ui->treeView->expandAll();

    // TODO: Make context configurable as well (hiding the column for now)
    ui->treeView->hideColumn(2);

    ui->treeView->setColumnWidth(0, 200);
    ui->treeView->resizeColumnToContents(1);

    ui->treeView->installEventFilter(this);

    timer->setSingleShot(true);
    connect(timer.get(), &QTimer::timeout, [this]() {
        releaseKeyboard();
        releaseMouse();
        QStandardItemModel* model = (QStandardItemModel*)ui->treeView->model();
        model->setData(last_index, last_index_value);
    });

    std::transform(Settings::values.buttons.begin(), Settings::values.buttons.end(),
                   buttons_param.begin(),
                   [](const std::string& str) { return Common::ParamPackage(str); });
    std::transform(Settings::values.analogs.begin(), Settings::values.analogs.end(),
                   analogs_param.begin(),
                   [](const std::string& str) { return Common::ParamPackage(str); });
}

ConfigureHotkeys::~ConfigureHotkeys() {}

bool ConfigureHotkeys::eventFilter(QObject* o, QEvent* e) {
    if (o == ui->treeView && e->type() == QEvent::KeyPress) {
        QModelIndexList list = ui->treeView->selectionModel()->selectedIndexes();
        QStandardItemModel* model = (QStandardItemModel*)ui->treeView->model();
        int row = -1;
        for (QModelIndex index : list) {
            if (index.row() != row && index.column() == 1) {
                QKeyEvent* key = static_cast<QKeyEvent*>(e);
                if (key->key() == Qt::Key_Control) {
                    if (timer->isActive() && last_index != index) {
                        QStandardItemModel* model = (QStandardItemModel*)ui->treeView->model();
                        model->setData(last_index, last_index_value);
                        timer->stop();
                    }
                    last_index = index;
                    last_index_value = model->data(last_index);
                    timer->start(2000); // Cancel after 2 seconds
                    model->setData(index, tr("Press another key (2sec)"));
                } else if (timer->isActive()) {
                    timer->stop();
                    if (!isUsedKey(QKeySequence(Qt::CTRL + key->key()))) {
                        model->setData(last_index, QKeySequence(Qt::CTRL + key->key()).toString());
                    }
                } else {
                    if (!isUsedKey(QKeySequence(key->key()))) {
                        model->setData(index, QKeySequence(key->key()).toString());
                    }
                }
            }
        }
    }
    return false;
}

const std::array<std::string, ConfigureHotkeys::ANALOG_SUB_BUTTONS_NUM>
    ConfigureHotkeys::analog_sub_buttons{{
        "up", "down", "left", "right", "modifier",
    }};

static QString getKeyName(int key_code) {
    switch (key_code) {
    case Qt::Key_Shift:
        return QObject::tr("Shift");
    case Qt::Key_Control:
        return QObject::tr("Ctrl");
    case Qt::Key_Alt:
        return QObject::tr("Alt");
    case Qt::Key_Meta:
        return "";
    default:
        return QKeySequence(key_code).toString();
    }
}

bool ConfigureHotkeys::isUsedKey(QKeySequence key_sequence) {

    // Check HotKeys
    QStandardItemModel* model = (QStandardItemModel*)ui->treeView->model();
    for (int r = 0; r < model->rowCount(); r++) {
        QStandardItem* parent = model->item(r, 0);
        for (int r2 = 0; r2 < parent->rowCount(); r2++) {
            QStandardItem* keyseq = parent->child(r2, 1);
            if (keyseq->text() == key_sequence.toString()) {
                return true;
            }
        }
    }

    // Check InputKeys
    QString non_keyboard(tr("[non-keyboard]"));

    auto KeyToText = [&non_keyboard](const Common::ParamPackage& param) {
        if (param.Get("engine", "") != "keyboard") {
            return non_keyboard;
        } else {
            return getKeyName(param.Get("code", 0));
        }
    };

    for (int button = 0; button < Settings::NativeButton::NumButtons; button++) {
        if (KeyToText(buttons_param[button]) == key_sequence.toString()) {
            return true;
        }
    }

    for (int analog_id = 0; analog_id < Settings::NativeAnalog::NumAnalogs; analog_id++) {
        if (analogs_param[analog_id].Get("engine", "") != "analog_from_button") {
            return false;
        } else {
            for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM; sub_button_id++) {
                Common::ParamPackage param(
                    analogs_param[analog_id].Get(analog_sub_buttons[sub_button_id], ""));
                if (KeyToText(param) == key_sequence.toString()) {
                    return true;
                }
            }
        }
    }

    return false;
}

void ConfigureHotkeys::applyConfiguration() {
    if (timer->isActive()) {
        QStandardItemModel* model = (QStandardItemModel*)ui->treeView->model();
        model->setData(last_index, last_index_value);
        timer->stop();
    }

    QStandardItemModel* model = (QStandardItemModel*)ui->treeView->model();
    for (int r = 0; r < model->rowCount(); r++) {
        QStandardItem* parent = model->item(r, 0);
        for (int r2 = 0; r2 < parent->rowCount(); r2++) {
            QStandardItem* action = parent->child(r2, 0);
            QStandardItem* keyseq = parent->child(r2, 1);
            for (HotkeyGroupMap::iterator it = hotkey_groups.begin(); it != hotkey_groups.end();
                 ++it) {
                if (it->first == parent->text()) {
                    for (HotkeyMap::iterator it2 = it->second.begin(); it2 != it->second.end();
                         ++it2) {
                        if (it2->first == action->text()) {
                            it2->second.keyseq = QKeySequence(keyseq->text());
                        }
                    }
                }
            }
        }
    }

    SaveHotkeys();
    Settings::Apply();
}
