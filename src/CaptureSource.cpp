/*
 * CaptureSource.cpp
 *
 *  Created on: Jun 18, 2011
 *      Author: bh
 */

#include "CaptureSource.h"
#include <QImageWriter>

Source::RTTI CaptureSource::type = Source::CAPTURE_SOURCE;

CaptureSource::CaptureSource(QImage capture, GLuint texture, double d): Source(texture, d) {

    setImage(capture);
    if (_capture.isNull())
        SourceConstructorException().raise();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

}

CaptureSource::~CaptureSource() {


}

QString CaptureSource::getInfo() const {

    return Source::getInfo() + tr(" - Pixmap ");
}


void CaptureSource::setImage(QImage capture)
{
    if ( !capture.isNull() ) {
        _capture = capture;
        _captureChanged = true;
    }
}

QDomElement CaptureSource::getConfiguration(QDomDocument &doc, QDir current)
{
    // get the config from proto source
    QDomElement sourceElem = Source::getConfiguration(doc, current);
    sourceElem.setAttribute("playing", isPlaying());
    QDomElement specific = doc.createElement("TypeSpecific");
    specific.setAttribute("type", rtti());

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);

    if (!QImageWriter::supportedImageFormats().count("jpeg")){
        qWarning() << getName() << QChar(124).toLatin1() << tr("Qt JPEG plugin not found; using XPM format (slower).") << QImageWriter::supportedImageFormats();
        if (!_capture.save(&buffer, "xpm") )
            qWarning() << getName() << QChar(124).toLatin1() << tr("Could not save pixmap source (XPM format).");
    }
    else
        if (!_capture.save(&buffer, "jpeg", 70) )
            qWarning() << getName()  << QChar(124).toLatin1() << tr("Could not save pixmap source (JPG format).");

    buffer.close();
    ba = ba.toBase64();
    QDomElement f = doc.createElement("Image");
    QDomText img = doc.createTextNode( QString::fromLatin1(ba.constData(), ba.size()) );

    f.appendChild(img);
    specific.appendChild(f);

    sourceElem.appendChild(specific);
    return sourceElem;
}



void CaptureSource::update()
{
    if (_captureChanged) {

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureIndex);

    #if QT_VERSION >= 0x040700
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  _capture.width(), _capture. height(),
                      0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _capture.constBits() );
    #else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  _capture.width(), _capture. height(),
                      0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _capture.bits() );
    #endif

        _captureChanged = false;
    }

    // perform source update
    Source::update();
}

