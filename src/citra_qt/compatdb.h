// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifndef COMPATDB_H
#define COMPATDB_H

#include <QWizard>

namespace Ui {
class CompatDB;
}

class CompatDB : public QWizard {
    Q_OBJECT

public:
    explicit CompatDB(QWidget* parent = 0);
    ~CompatDB();

private:
    Ui::CompatDB* ui;

private slots:
    void Submit();
};

#endif // COMPATDB_H
