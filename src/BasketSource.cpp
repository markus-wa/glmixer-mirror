#include "BasketSource.moc"

#include "ViewRenderWidget.h"
#include <QImage>

Source::RTTI BasketSource::type = Source::BASKET_SOURCE;
bool BasketSource::playable = true;

BasketSource::BasketSource(QStringList files, double d, int w, int h, qint64 p) : Source(0, d), width(w), height(h), period(p), bidirectional(false), shuffle(false), _pause(false), _renderFBO(0), _atlasFBO(0), _atlasInitialized(false), _indexPlaylist(0)
{

    // allocate the FBO for rendering
    _renderFBO = new QGLFramebufferObject(width, height);
    CHECK_PTR_EXCEPTION(_renderFBO);

    // fill in the atlas
    appendImages(files);

    // set a default playlist
    for (int i = 0; i < _atlasImages.size(); ++i) {
        _playlist.append(i);
    }

    // if invalid period given, set to default 40Hz
    if (period <= 10)
        period = 24;
    _timer.start();
}

BasketSource::~BasketSource() {

    if(_renderFBO)
        delete _renderFBO;
    if (_atlasFBO)
        delete _atlasFBO;
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


QStringList BasketSource::getImageFileList() const {

    QStringList list;

    foreach (BasketImage img, _atlasImages) {
        QString completefilename = QFileInfo( img.fileName() ).absoluteFilePath();
        list.append(completefilename);
    }

    return list;
}

void BasketSource::update() {

    if (!_pause) {

        if (_timer.elapsed() > period)
        {

            // select BasketImage index from playlist
            _indexPlaylist = (_indexPlaylist+1)%(_playlist.size()); // ordered increment
            QRect r = _atlasImages[_playlist[_indexPlaylist]].coordinates();

            //        qDebug()<< "update basket" << r << _indexPlaylist;

            // blit part of atlas to render FBO
            // use the accelerated GL_EXT_framebuffer_blit if available
            if (RenderingManager::useFboBlitExtension())
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, _atlasFBO->handle());
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _renderFBO->handle());
                glBlitFramebuffer(r.x(), r.y(), r.x() + r.width(), r.y() + r.height(),
                                  0, 0, width, height,
                                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            }
            // TODO : non blit render

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

BasketImage::BasketImage(QString f) : _fileName(f), _filled(false)
{

}

void BasketSource::appendImages(QStringList files){

    // fill in the images basket
    foreach (QString file, files) {
        _atlasImages.append(BasketImage(file));
    }

    // allocate the FBO for the atlas
    QSize array = allocateAtlas(_atlasImages.size());

    // create a texture for filling images to atlas
    GLuint tex = 0;
    glGenTextures(1, &tex);

    // fill in the atlas
    QPoint index;
    if (_atlasFBO->bind()) {

        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_VIEWPORT_BIT);

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glColor4f(1.0, 1.0, 1.0, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(-1.0, 1.0, 1.0, -1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        for (int i = 0; i < _atlasImages.size(); ++i) {

            // set rendering area
            QRect coords(index.x()*width, index.y()*height, width, height);
            glViewport(coords.x(), coords.y(), coords.width(), coords.height() );

            // remember coordinates of image in FBO
            _atlasImages[i].setCoordinates(coords);

            // render image in FBO
            QImage image(_atlasImages[i].fileName());
#if QT_VERSION >= 0x040700
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  image.width(), image. height(),
                          0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.constBits() );
#else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  image.width(), image. height(),
                          0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.bits() );
#endif
            glCallList(ViewRenderWidget::quad_texured);

            // next index
            if (index.x()<array.width())
                // next column
                index += QPoint(1,0);
            else
                // retour a la ligne
                index = QPoint(0, index.y() + 1);

        }

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glPopAttrib();

        _atlasFBO->release();
    }
    else
        FboRenderingException().raise();

    // remove texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &tex);

    // TODO : smart copy of previous atlas FBO (blit)
    // instead of reloading all
}

QSize BasketSource::allocateAtlas(int n) {

    // Check limits of the openGL texture
    GLint maxtexturewidth = TEXTURE_REQUIRED_MAXIMUM;
    if (glewIsSupported("GL_ARB_internalformat_query2"))
        glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_MAX_WIDTH, 1, &maxtexturewidth);
    else
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexturewidth);
    maxtexturewidth = qMin(maxtexturewidth, GL_MAX_FRAMEBUFFER_WIDTH);


    // compute number of lines necessary for packing n images
    QSize array;
    int w = width * n;
    array.setHeight( w / maxtexturewidth  + 1 );
    // number of columns
    array.setWidth( n / array.height() );

//    qDebug()<< "Atlas dimensions " << n << array << array.width() * width << array.height() * height;

    // reallocate FBO Atlas
    if (_atlasFBO)
        delete _atlasFBO;
    _atlasFBO = new QGLFramebufferObject(array.width() * width, array.height() * height);
    CHECK_PTR_EXCEPTION(_atlasFBO);

    return array;
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

    // list of files in basket
    QDomElement basket = doc.createElement("Images");
    foreach (BasketImage img, _atlasImages) {

        QDomElement f = doc.createElement("Filename");
        QString completefilename = QFileInfo( img.fileName() ).absoluteFilePath();
        if (current.isReadable())
            f.setAttribute("Relative", current.relativeFilePath( completefilename ) );
        QDomText filename = doc.createTextNode( completefilename );
        f.appendChild(filename);
        basket.appendChild(f);
    }
    specific.appendChild(basket);

    sourceElem.appendChild(specific);
    return sourceElem;
}
