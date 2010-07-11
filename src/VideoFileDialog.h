/*
 * videoFileDialog.h
 *
 *  Created on: Aug 3, 2009
 *      Author: bh
 */

#ifndef VIDEOFILEDIALOG_H_
#define VIDEOFILEDIALOG_H_

#include <QFileDialog>
#include <QCheckBox>

#include "VideoFileDialogPreview.h"

class VideoFileDialog: public QFileDialog
{
    Q_OBJECT

public:
    VideoFileDialog( QWidget * parent = 0, const QString & caption = QString(), const QString & directory = QString(), const QString & filter = QString() );
    ~VideoFileDialog();

    bool configCustomSize();

public Q_SLOTS:
    void setPreviewVisible(bool visible);

signals:
    /**
     * Signal emmited when an error occurs;
     * The VideoFile should gently stop, but may stay in a non-usable state; better stop it, or delete it.
     *
     * @param msg Error message
     */
    void error(QString msg);


private:

    VideoFileDialogPreview *preview;
    QCheckBox *pv;

};

#endif /* VideoFileDialog_H_ */
