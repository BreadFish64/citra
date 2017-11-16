// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra_qt/configuration/configure_graphics.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_graphics.h"

#include <QColorDialog>

ConfigureGraphics::ConfigureGraphics(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureGraphics) {

    ui->setupUi(this);
    this->setConfiguration();

    ui->toggle_vsync->setEnabled(!Core::System::GetInstance().IsPoweredOn());

    ui->layoutBox->setEnabled(!Settings::values.custom_layout);

    connect(ui->layout_bg, SIGNAL (released()), this, SLOT (showLayoutBackgroundDialog()));
}

ConfigureGraphics::~ConfigureGraphics() {}

void ConfigureGraphics::showLayoutBackgroundDialog() {
    QColor new_color = QColorDialog::getColor(bg_color, this);
    if (new_color.isValid()) {
        bg_color = new_color;
        ui->layout_bg->setStyleSheet("QPushButton { background-color: " + bg_color.name() + ";}");
    }
}

void ConfigureGraphics::setConfiguration() {
    ui->toggle_hw_renderer->setChecked(Settings::values.use_hw_renderer);
    ui->resolution_factor_combobox->setEnabled(Settings::values.use_hw_renderer);
    ui->toggle_shader_jit->setChecked(Settings::values.use_shader_jit);
    ui->resolution_factor_combobox->setCurrentIndex(Settings::values.resolution_factor);
    ui->toggle_vsync->setChecked(Settings::values.use_vsync);
    {
        bg_color.setRgbF(Settings::values.bg_red, Settings::values.bg_green, Settings::values.bg_blue);
        ui->layout_bg->setStyleSheet("QPushButton { background-color: " + bg_color.name() + ";}");
    }
    ui->toggle_framelimit->setChecked(Settings::values.toggle_framelimit);
    ui->layout_combobox->setCurrentIndex(static_cast<int>(Settings::values.layout_option));
    ui->swap_screen->setChecked(Settings::values.swap_screen);
}

void ConfigureGraphics::applyConfiguration() {
    Settings::values.use_hw_renderer = ui->toggle_hw_renderer->isChecked();
    Settings::values.use_shader_jit = ui->toggle_shader_jit->isChecked();
    Settings::values.resolution_factor =
        static_cast<u16>(ui->resolution_factor_combobox->currentIndex());
    Settings::values.use_vsync = ui->toggle_vsync->isChecked();
    Settings::values.bg_red = bg_color.redF();
    Settings::values.bg_green = bg_color.greenF();
    Settings::values.bg_blue = bg_color.blueF();
    Settings::values.toggle_framelimit = ui->toggle_framelimit->isChecked();
    Settings::values.layout_option =
        static_cast<Settings::LayoutOption>(ui->layout_combobox->currentIndex());
    Settings::values.swap_screen = ui->swap_screen->isChecked();
    Settings::Apply();
}
