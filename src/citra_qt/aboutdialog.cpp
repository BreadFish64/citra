// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

#include "citra_qt/aboutdialog.h"
#include "common/scm_rev.h"

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("About Citra"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QString text = QStringLiteral("");
    QString small = QStringLiteral("<p style='margin-top:0; margin-bottom:0; font-size:small;'>");
    QString large = QStringLiteral("<p style='margin-top:0; margin-bottom:0; font-size:large;'>");

    text.append(QStringLiteral("<p style='font-size:38pt; font-weight:400; margin-bottom:0;'>") +
                tr("Citra") + QStringLiteral("</p>"));
    text.append(QStringLiteral("<p style='font-size:18pt; margin-top:0;'>%1</p>")
                    .arg(QString::fromUtf8(Common::g_build_name)));

    text.append(small + tr("Branch: ") + QString::fromUtf8(Common::g_scm_branch) +
                QStringLiteral("</p>"));
    text.append(small + tr("Revision: ") + QString::fromUtf8(Common::g_scm_desc) +
                QStringLiteral("</p>"));

    text.append(large +
                tr("<br/>Citra is a free and open source 3DS emulator licensed under GPLv2.0 or "
                   "any later version.") +
                QStringLiteral("</p><br/>"));
    text.append(large +
                tr("This software should not be used to play games you do not legally own.") +
                QStringLiteral("</p><br/>"));
    text.append(large +
                QStringLiteral(
                    "<a href='https://citra-emu.org/'>%1</a> | "
                    "<a href='https://community.citra-emu.org/'>%2</a> | "
                    "<a href='https://github.com/citra-emu'>%3</a> | "
                    "<a href='https://github.com/citra-emu/citra/graphs/contributors'>%4</a> | "
                    "<a "
                    "href='https://github.com/citra-emu/citra/blob/master/license.txt'>%5</a></p>")
                    .arg(tr("Website"))
                    .arg(tr("Forum"))
                    .arg(tr("Source Code"))
                    .arg(tr("Contributors"))
                    .arg(tr("License")));

    QLabel* text_label = new QLabel(text);
    text_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    text_label->setOpenExternalLinks(true);

    QLabel* copyright = new QLabel(small + tr("\"3DS\" is a trademark of Nintendo. Citra is not "
                                              "affiliated with Nintendo in any way.") +
                                   QStringLiteral("</p>"));

    QLabel* logo = new QLabel();
    logo->setPixmap(QPixmap(":/icons/citra.png"));
    logo->setContentsMargins(30, 0, 30, 0);

    QVBoxLayout* about_layout = new QVBoxLayout;
    QHBoxLayout* h_layout = new QHBoxLayout;

    setLayout(about_layout);
    about_layout->addLayout(h_layout);
    about_layout->addWidget(copyright);
    copyright->setAlignment(Qt::AlignCenter);
    copyright->setContentsMargins(0, 15, 0, 0);

    h_layout->setAlignment(Qt::AlignLeft);
    h_layout->addWidget(logo);
    h_layout->addWidget(text_label);
}
