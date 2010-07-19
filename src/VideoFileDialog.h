/*
 * videoFileDialog.h
 *
 *  Created on: Aug 3, 2009
 *      Author: bh
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2010 Bruno Herbelin
 *
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
