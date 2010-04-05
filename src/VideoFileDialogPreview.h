/*
 * videoFileDialogPreview.h
 *
 *  Created on: Aug 3, 2009
 *      Author: bh
 */

#ifndef VIDEOFILEDIALOGPREVIEW_H_
#define VIDEOFILEDIALOGPREVIEW_H_

#include <QWidget>
#include "ui_VideoFileDialog.h"
#include "VideoFile.h"

class VideoFileDialogPreview: public QWidget, public Ui::VideoFileDialogPreviewWidget
{
    Q_OBJECT

public:
    VideoFileDialogPreview(QWidget *parent = 0);
    ~VideoFileDialogPreview();

public Q_SLOTS:
    void showFilePreview(const QString & file);
    void on_customSizeCheckBox_toggled(bool);

signals:
    /**
     * Signal emmited when an error occurs;
     * The VideoFile should gently stop, but may stay in a non-usable state; better stop it, or delete it.
     *
     * @param msg Error message
     */
    void error(QString msg);

private:

    VideoFile *is;
};

#endif /* VideoFileDialogPreview_H_ */
