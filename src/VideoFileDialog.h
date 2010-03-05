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

    static bool configPreviewVisible() {
        return _configPreviewVisible;
    }
    static void configSetPreviewVisible(bool visible) {
        _configPreviewVisible = visible;
    }
    static bool configCustomSize() {
        return _configCustomSize;
    }
    static void configSetCustomSize(bool c) {
        _configCustomSize = c;
    }


public slots:
    void setPreviewVisible(bool visible);
    void rememberFilter(const QString &);

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

    static bool _configPreviewVisible, _configCustomSize;
    static QString _filterSelected;

};

#endif /* VideoFileDialog_H_ */
