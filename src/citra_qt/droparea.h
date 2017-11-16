#ifndef DROPAREA_H
#define DROPAREA_H

#include <QLabel>

class QMimeData;

class DropArea : public QLabel
{
    Q_OBJECT

public:
    DropArea(QWidget *parent = 0);
    QString GetImageFilepath();
    void SetImage(QString filepath);

public slots:
    void clear();

signals:
    void changed(const QMimeData *mimeData = 0);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    QString imageFilepath;
    QLabel *label;
};

 #endif