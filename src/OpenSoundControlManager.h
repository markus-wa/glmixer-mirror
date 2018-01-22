/*
 * OpenSoundControlManager.h
 *
 *  Created on: Jun 29, 2009
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
 *   Copyright 2009, 2016 Bruno Herbelin
 *
 */

#ifndef OPENSOUNDCONTROLMANAGER_H
#define OPENSOUNDCONTROLMANAGER_H

#include <QString>
#include <QUdpSocket>

#define OSC_VOID "void"
#define OSC_VOID_LOG "log"
#define OSC_VOID_IGNORE "ignore"
#define OSC_RENDER "render"
#define OSC_RENDER_ALPHA "alpha"
#define OSC_RENDER_TRANSPARENCY "transparency"
#define OSC_RENDER_PAUSE "pause"
#define OSC_RENDER_UNPAUSE "unpause"
#define OSC_RENDER_NEXT "next"
#define OSC_RENDER_PREVIOUS "previous"
#define OSC_SOURCE_PRESET "preset"
#define OSC_SOURCE_CURRENT "current"
#define OSC_SOURCE_PLAY "play"
#define OSC_SOURCE_PAUSE "pause"
#define OSC_SOURCE_LOOP "loop"
#define OSC_SOURCE_SPEED "speed"
#define OSC_SOURCE_FASTFORWARD "fastforward"
#define OSC_SOURCE_TIMING "timing"
#define OSC_SELECT "select"
#define OSC_SELECT_NEXT "next"
#define OSC_SELECT_PREVIOUS "previous"
#define OSC_SELECT_NONE "none"
#define OSC_REQUEST "request"
#define OSC_REQUEST_COUNT "count"
#define OSC_REQUEST_NAME "name"
#define OSC_REQUEST_CURRENT "current"
#define OSC_REQUEST_CONNECT "connection"


/**
 * Open Sound Control network server (UDP) to receive commands
 * from external programs.
 *
 * OSC messages sent to the OpenSoundControlManager have to be in the form:
 *
 *    /glmixer/<object>/<property>
 *
 * Where
 *   -  <object> is the name of the object to modify.
 *      It can either be 'render' to affect the renderer
 *      or it can be the name of a source (currently existing in the session)
 *
 *   -  <property> is the label of the parameter to change
 *      It can for example be 'Alpha' to change the alpha value of a source.
 *      The label of the property is the same as in the property browser
 *
 * Examples:
 *
 *   -  /glmixer/render/Alpha
 *      Change the alpha value of the rendering window
 *
 *   -  /glmixer/sourceName/Alpha
 *      Change the transparency of the source named 'sourceName'
 *
 *   -  /glmixer/sourceName/Contrast
 *      Change the contrast of the source named 'sourceName'
 *
 *
 * The OSC message is then followed by the list of required arguments (values).
 * Values can be of type int, float or bool, depending on the need.
 *
 * OpenSoundControlManager uses a local copy of libOSCPack from:
 * http://www.rossbencina.com/code/oscpack
 *
 * OpenSoundControl 1.0 is described here:
 * http://opensoundcontrol.org/spec-1_0
 */
class OpenSoundControlManager: public QObject
{
    Q_OBJECT

public:
    static OpenSoundControlManager *getInstance();

    // server UDP
    void setEnabled(bool enable, qint16 portreceive, qint16 portbroadcast);
    bool isEnabled();
    qint16 getPortReceive();
    qint16 getPortBroadcast();

    void broadcastDatagram(QString property, QVariantList args = QVariantList());
    void executeMessage(QString pattern, QVariantList args);

    // translator
    void addTranslation(QString before, QString after);
    bool hasTranslation(QString before, QString after);
    QList< QPair<QString, QString> > *getTranslationDictionnary() const { return _dictionnary; }

    // logs
    void setVerbose(bool on) { _verbose = on; }
    bool isVerbose() const { return _verbose; }

public slots:

    void readPendingDatagrams();
    void broadcastSourceCount(int count);

signals:
    void log(QString);
    void error(QString);
    void applyPreset(QString, QString);

private:
    OpenSoundControlManager();
    static OpenSoundControlManager *_instance;

    void execute(QString object, QString property, QVariantList args);
    void executeSource(class Source *s, QString property, QVariantList args);
    void executeRender(QString property, QVariantList args);
    void executeRequest(QString property, QVariantList args);

    QUdpSocket *_udpReceive;
    QUdpSocket *_udpBroadcast;
    qint16 _portReceive, _portBroadcast;

    QList< QPair<QString, QString> > *_dictionnary;
    bool _verbose;
};

#endif // OPENSOUNDCONTROLMANAGER_H
