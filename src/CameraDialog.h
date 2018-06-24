/*
 * CameraDialog.h
 *
 *  Created on: Dec 19, 2009
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
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef CAMERADIALOG_H_
#define CAMERADIALOG_H_

#include <QDialog>
#include <QSettings>

namespace Ui {
class CameraDialog;
}

class CameraDialog : public QDialog
{
    Q_OBJECT

public:
    CameraDialog(QWidget *parent = 0, QSettings *settings = 0);
    virtual ~CameraDialog();

#ifdef GLM_OPENCV
    bool isOpencvSelected() const;
    int getOpencvIndex() const;
#endif

    QString getUrl() const;
    QString getFormat() const;
    QHash<QString, QString> getFormatOptions() const;

public slots:
    void done(int r);

    void setScreenCaptureArea(int index);
    void updateScreenCaptureArea();
    void updateSourcePreview();
    void cancelSourcePreview();
    void connectedInfo();
    void failedInfo();
    void showHelp();

protected:
    void showEvent(QShowEvent *);

private:
    Ui::CameraDialog *ui;

    class Source *s;
    QTimer *testingtimeout, *respawn;
    QSettings *appSettings;
    QRect screendimensions;
};

#endif /* CAMERADIALOG_H_ */

/*
#ifdef GLM_OPENCV
    void setOpencvCamera(int i);

public:
    inline int indexOpencvCamera() const {return currentCameraIndex;}
    int modeOpencvCamera() const;

private:
    int currentCameraIndex;
#endif
*/
