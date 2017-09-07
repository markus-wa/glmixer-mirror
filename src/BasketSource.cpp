#include "BasketSource.moc"

#include "ViewRenderWidget.h"

#include <QImage>
//#include <algorithm>
#include "ImageAtlas.h"

Source::RTTI BasketSource::type = Source::BASKET_SOURCE;
bool BasketSource::playable = true;

BasketSource::BasketSource(QStringList files, double d, int w, int h, qint64 p) : Source(0, d),  period(p), bidirectional(false), shuffle(false), _pause(false), _renderFBO(0)
{
    // allocate the FBO for rendering
    _renderFBO = new QGLFramebufferObject(w, h);
    CHECK_PTR_EXCEPTION(_renderFBO);

    // fill in the atlas
    _atlas.setSize(w, h);
    if (!_atlas.appendImages(files))
         FboRenderingException().raise();

    // set a default playlist
    QList<int> defaultlist;
    for (int i = 0; i < files.size(); ++i) {
        defaultlist.append(i);
    }
    setPlaylist(defaultlist);

    // if invalid period given, set to default 40Hz
    if (period <= 10)
        period = 24;
    _timer.start();
}

BasketSource::~BasketSource() {

    if(_renderFBO)
        delete _renderFBO;

}


GLuint BasketSource::getTextureIndex() const {

    if (_renderFBO)
        return _renderFBO->texture();
    else
        return Source::getTextureIndex();
}

int BasketSource::getFrameWidth() const {

    if (_renderFBO)
        return _renderFBO->width();
    else
        return Source::getFrameWidth();
}

int BasketSource::getFrameHeight() const {

    if (_renderFBO)
        return _renderFBO->height();
    else
        return Source::getFrameHeight();
}

double BasketSource::getFrameRate() const {
    return 1.0 / ((double)(period) / 1000.0);
}

void BasketSource::play(bool on) {

    _pause = !on;

    Source::play(on);
}

bool BasketSource::isPlaying() const {

    return !_pause;
}

bool BasketSource::isBidirectional() const {

    return bidirectional;
}

void BasketSource::setBidirectional(bool on) {

    bidirectional = on;
}

bool BasketSource::isShuffle() const {

    return shuffle;
}

void BasketSource::setShuffle(bool on) {

    shuffle = on;
}

qint64 BasketSource::getPeriod() const {

    return period;
}

void BasketSource::setPeriod(qint64 p){

    period = p;
}


void BasketSource::setPlaylist(QList<int> playlist){

    _playlist.clear();
    _executionList.clear();

    // copy the given playlist
    // but with a validation check for validity of the indices
    foreach (int index, playlist) {
        if (index > -1 && index < _atlas.count())
            _playlist.append(index);
    }

}


void BasketSource::generateExecutionPlaylist(){

    // take the playlist and apply execution options
    _executionList = _playlist;

    if (shuffle) {
        std::random_shuffle(_executionList.begin(), _executionList.end());
    }

    if (bidirectional) {
        QList<int> reverse;
        reverse.reserve( _executionList.size() ); // reserve is new in Qt 4.7
        std::reverse_copy( _executionList.begin(), _executionList.end(), std::back_inserter( reverse ) );
        reverse.takeFirst();
        reverse.takeLast();
        _executionList += reverse;
    }
}

QStringList BasketSource::getImageFileList() const {

    return _atlas.getImageList();
}


QList<int> BasketSource::getPlaylist() const {

    return _playlist;
}


void BasketSource::update() {

    if (!_pause) {

        if (_timer.elapsed() > period)
        {
            if (_executionList.isEmpty())
                generateExecutionPlaylist();

            // select BasketImage index from execution playlist
            int index = _executionList.takeFirst();
            QRect r = _atlas[index].coordinates();
            QGLFramebufferObject *fbo = _atlas[index].page()->fbo();

            // blit part of atlas to render FBO
            // use the accelerated GL_EXT_framebuffer_blit if available
            if (RenderingManager::useFboBlitExtension())
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo->handle());
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFBO->handle());
                glBlitFramebuffer(r.x(), r.y(), r.x() + r.width(), r.y() + r.height(),
                                  0, 0, _atlas.size().width(), _atlas.size().height(),
                                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            }
            // TODO : else non-blit render

            _timer.restart();
        }

    }

    // TODO : smart refill of atlas if necessary (not _initialized)
    // if not filled, fill in the BasketImage in the atlas
//    if (_atlasFBO->bind()) {
//        _atlasFBO->release();
//    }


    // perform source update
    Source::update();
}


void BasketSource::appendImages(QStringList files){

    _atlas.appendImages(files);

}


QDomElement BasketSource::getConfiguration(QDomDocument &doc, QDir current)
{
    // get the config from proto source
    QDomElement sourceElem = Source::getConfiguration(doc, current);
    sourceElem.setAttribute("playing", isPlaying());
    QDomElement specific = doc.createElement("TypeSpecific");
    specific.setAttribute("type", rtti());

    QDomElement s = doc.createElement("Frame");
    s.setAttribute("Width", getFrameWidth());
    s.setAttribute("Height", getFrameHeight());
    specific.appendChild(s);

    QDomElement x = doc.createElement("Update");
    x.setAttribute("Periodicity", QString::number(period) );
    specific.appendChild(x);

    QDomElement pl = doc.createElement("Playlist");
    QStringList list;
    foreach(int i, _playlist)
        list.append(QString::number(i));
    pl.appendChild( doc.createTextNode( list.join(",") ) );
    pl.setAttribute("Bidirectional", bidirectional );
    pl.setAttribute("Shuffle", shuffle );
    specific.appendChild(pl);

    // list of files in basket
    QDomElement basket = doc.createElement("Images");
    foreach (QString filename, _atlas.getImageList()) {
        QDomElement f = doc.createElement("Filename");
        QString completefilename = QFileInfo( filename ).absoluteFilePath();
        if (current.isReadable())
            f.setAttribute("Relative", current.relativeFilePath( completefilename ) );
        f.appendChild( doc.createTextNode( completefilename ));
        basket.appendChild(f);
    }
    specific.appendChild(basket);

    sourceElem.appendChild(specific);
    return sourceElem;
}


