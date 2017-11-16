// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>
#include <QFutureWatcher>

namespace Ui {
class DirectConnect;
}

class DirectConnectWindow : public QDialog {
    Q_OBJECT

public:
    explicit DirectConnectWindow(QWidget* parent = nullptr);

private:
    void Connect();
    void ClearAllError();
    void BeginConnecting();
    void EndConnecting();

private slots:
    void OnConnection();

private:
    QFutureWatcher<void>* watcher;
    Ui::DirectConnect* ui;
};
