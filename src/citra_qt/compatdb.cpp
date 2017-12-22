// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QMessageBox>
#include <QPushButton>
#include "common/telemetry.h"
#include "compatdb.h"
#include "core/core.h"
#include "ui_compatdb.h"

CompatDB::CompatDB(QWidget* parent)
    : QWizard(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint),
      ui(new Ui::CompatDB) {
    ui->setupUi(this);
    QList<QWizard::WizardButton> layout1;
    layout1 << QWizard::Stretch << QWizard::BackButton << QWizard::CustomButton1
            << QWizard::CancelButton << QWizard::FinishButton;
    this->setButtonLayout(layout1);
    this->setButtonText(QWizard::CustomButton1, tr("Next"));
    connect(this->button(QWizard::CustomButton1), &QPushButton::clicked, this, &CompatDB::Submit);
}

void CompatDB::Submit() {
    if (this->currentId() == 1) {
        QButtonGroup* compatibility = new QButtonGroup(this);
        compatibility->addButton(ui->radioButton_Perfect, 0);
        compatibility->addButton(ui->radioButton_Great, 1);
        compatibility->addButton(ui->radioButton_Okay, 2);
        compatibility->addButton(ui->radioButton_Bad, 3);
        compatibility->addButton(ui->radioButton_IntroMenu, 4);
        compatibility->addButton(ui->radioButton_WontBoot, 5);
        LOG_DEBUG(
            Frontend,
            tr("Compatibility Rating: %1").arg(compatibility->checkedId()).toStdString().c_str());
        if (compatibility->checkedId() != -1) {
            Core::Telemetry().AddField(Telemetry::FieldType::UserFeedback, "Compatibility",
                                       compatibility->checkedId());
            delete compatibility;
            QList<QWizard::WizardButton> layout2;
            layout2 << QWizard::Stretch << QWizard::FinishButton;
            next();
            setButtonLayout(layout2);
        } else {
            QMessageBox::critical(this, tr("No Rating Selected"),
                                  tr("Please select a compatibilty rating."));
            delete compatibility;
        }
    } else {
        next();
    }
}

CompatDB::~CompatDB() {
    delete ui;
}
