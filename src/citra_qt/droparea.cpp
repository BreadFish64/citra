#include <QtGui>

#include "droparea.h"

DropArea::DropArea(QWidget *parent) : QLabel(parent) {
    setMinimumSize(200, 120);
    setMaximumSize(200, 120);
    setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
    setAlignment(Qt::AlignCenter);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    clear();
}

void DropArea::dragEnterEvent(QDragEnterEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    setBackgroundRole(QPalette::Highlight);
    if (mimeData->hasImage() || mimeData->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DropArea::dragMoveEvent(QDragMoveEvent *event) {
    event->acceptProposedAction();
}

void DropArea::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        SetImage(urlList.at(0).toLocalFile());
    } else {
        setText(tr("Invalid image file"));
    }

    setBackgroundRole(QPalette::Dark);
    event->acceptProposedAction();
}

QString DropArea::GetImageFilepath() {
    return imageFilepath;
}

void DropArea::SetImage(QString filepath) {
    imageFilepath = filepath;
    QPixmap img = QPixmap(imageFilepath);
    if(!img.isNull()){
        setPixmap(img.scaled(200, 120, Qt::KeepAspectRatio));
        setToolTip(imageFilepath);
    }
}

void DropArea::dragLeaveEvent(QDragLeaveEvent *event)  {
    event->accept();
}

void DropArea::clear(){
    setText(tr("<Drop an image>"));
    setBackgroundRole(QPalette::Dark);

    emit changed();
}