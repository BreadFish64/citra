// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QtGui>
#include "citra_qt/configuration/configure_camera.h"
#include "core/settings.h"
#include "ui_configure_camera.h"

ConfigureCamera::ConfigureCamera(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureCamera>()) {
    ui->setupUi(this);

    ui->camera_outer_right_mode->clear();
    ui->camera_outer_right_mode->addItem("blank");
    ui->camera_outer_right_mode->addItem("image");

    ui->camera_outer_left_mode->clear();
    ui->camera_outer_left_mode->addItem("blank");
    ui->camera_outer_left_mode->addItem("image");

    ui->camera_inner_mode->clear();
    ui->camera_inner_mode->addItem("blank");
    ui->camera_inner_mode->addItem("image");

    //    connect(ui->camera_outer_right_drop_area, SIGNAL(changed(const QMimeData*)), this, SLOT (OuterRightCameraDrop(const QMimeData*)));
    //    connect(ui->camera_outer_left_drop_area, SIGNAL(changed(const QMimeData*)), this, SLOT (OuterLeftCameraDrop(const QMimeData*)));
    //    connect(ui->camera_inner_drop_area, SIGNAL(changed(const QMimeData*)), this, SLOT (InnerCameraDrop(const QMimeData*)));

    connect(ui->camera_outer_right_mode, SIGNAL(currentIndexChanged(int)), this, SLOT (OuterRightCameraModeChanged(int)));
    connect(ui->camera_outer_left_mode, SIGNAL(currentIndexChanged(int)), this, SLOT (OuterLeftCameraModeChanged(int)));
    connect(ui->camera_inner_mode, SIGNAL(currentIndexChanged(int)), this, SLOT (InnerCameraModeChanged(int)));

    this->setConfiguration();
}

ConfigureCamera::~ConfigureCamera() {}

void ConfigureCamera::OuterRightCameraModeChanged(int index){
     if(index){
         ui->camera_outer_right_drop_area->show();
     } else {
         ui->camera_outer_right_drop_area->hide();
     }
 }

 void ConfigureCamera::OuterLeftCameraModeChanged(int index){
     if(index){
         ui->camera_outer_left_drop_area->show();
     } else {
         ui->camera_outer_left_drop_area->hide();
     }
 }

 void ConfigureCamera::InnerCameraModeChanged(int index){
     if(index){
         ui->camera_inner_drop_area->show();
     } else {
         ui->camera_inner_drop_area->hide();
     }
 }

void ConfigureCamera::setConfiguration() {
    using namespace Service::CAM;

    // outer right camera
    if (ui->camera_outer_right_mode->itemText(0).toStdString() == Settings::values.camera_name[OuterRightCamera]) {
         ui->camera_outer_right_mode->setCurrentIndex(0);
         ui->camera_outer_right_drop_area->hide();
    } else {
        ui->camera_outer_right_mode->setCurrentIndex(1);
        ui->camera_outer_right_drop_area->show();
    }
    ui->camera_outer_right_drop_area->SetImage(QString::fromStdString(Settings::values.camera_config[OuterRightCamera]));

    // outer left camera
    if (ui->camera_outer_left_mode->itemText(0).toStdString() ==
        Settings::values.camera_name[OuterLeftCamera]) {
        ui->camera_outer_left_mode->setCurrentIndex(0);
        ui->camera_outer_left_drop_area->hide();
    } else {
        ui->camera_outer_left_mode->setCurrentIndex(1);
        ui->camera_outer_left_drop_area->show();
    }
        ui->camera_outer_left_drop_area->SetImage(QString::fromStdString(Settings::values.camera_config[OuterLeftCamera]));

    // inner camera
    if (ui->camera_inner_mode->itemText(0).toStdString() ==
        Settings::values.camera_name[InnerCamera]) {
         ui->camera_inner_mode->setCurrentIndex(0);
         ui->camera_inner_drop_area->hide();
    } else {
         ui->camera_inner_mode->setCurrentIndex(1);
         ui->camera_inner_drop_area->show();
    }
         ui->camera_inner_drop_area->SetImage(QString::fromStdString(Settings::values.camera_config[InnerCamera]));
}


void ConfigureCamera::applyConfiguration() {
    using namespace Service::CAM;

    // outer right camera
    Settings::values.camera_name[OuterRightCamera] =
        ui->camera_outer_right_mode->itemText(ui->camera_outer_right_mode->currentIndex())
            .toStdString();

    Settings::values.camera_config[OuterRightCamera] =
    ui->camera_outer_right_drop_area->GetImageFilepath().toStdString();

    // outer left camera
    Settings::values.camera_name[OuterLeftCamera] =
        ui->camera_outer_left_mode->itemText(ui->camera_outer_left_mode->currentIndex())
            .toStdString();
    Settings::values.camera_config[OuterLeftCamera] =
    ui->camera_outer_left_drop_area->GetImageFilepath().toStdString();

    // inner camera
    Settings::values.camera_name[InnerCamera] =
        ui->camera_inner_mode->itemText(ui->camera_inner_mode->currentIndex()).toStdString();
     Settings::values.camera_config[InnerCamera] =
        ui->camera_inner_drop_area->GetImageFilepath().toStdString();

    Settings::Apply();
}