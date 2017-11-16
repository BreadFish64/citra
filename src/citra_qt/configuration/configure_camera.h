// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureCamera;
}

class ConfigureCamera : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureCamera(QWidget* parent = nullptr);
    ~ConfigureCamera();

    void applyConfiguration();

private slots:
    void OuterRightCameraModeChanged(int index);
    void OuterLeftCameraModeChanged(int index);
    void InnerCameraModeChanged(int index);

private:
    void setConfiguration();

    std::unique_ptr<Ui::ConfigureCamera> ui;
};