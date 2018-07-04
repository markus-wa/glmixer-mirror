#include "ImageAtlas.moc"

#include "common.h"
#include "ViewRenderWidget.h"
#include "VideoFile.h"

#include <QImage>

ImageAtlasElement::ImageAtlasElement(QString filename): _fileName(filename)
{
    _page = NULL;
}

QStringList ImageAtlas::getImageList() const {

    QStringList list;

    foreach (ImageAtlasElement img, _atlasElements) {
        list.append(img.fileName());
    }

    return list;
}

ImageAtlas::~ImageAtlas() {

    qDeleteAll(_atlasPages);
}

void ImageAtlas::setSize(int w, int h) {
    _elementsSize = QSize(w, h);
    _initialized = true;
}

bool ImageAtlas::appendImages(QStringList files)
{
    if (!_initialized)
        return false;

    // ignore obvious
    if (files.empty())
        return false;

    int numElementsAtlas = _atlasElements.count();

    // create a temporary texture for filling images to atlas
    GLuint tex = 0;
    glGenTextures(1, &tex);

    // how many elements to insert
    int c = files.count();
    do {
        // create a page of atlas
        ImageAtlasPage *atlaspage = new ImageAtlasPage(_elementsSize, c);
        _atlasPages.append( atlaspage );

        // fill the fbo
        if (atlaspage->fbo()->bind()) {

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

            QPoint index;
            for (int i = 0; i < atlaspage->count(); ++i) {

                // create atlas element with filename
                ImageAtlasElement e(files.takeFirst());

                // try to make an image
                VideoFile mediafile;
                if ( !mediafile.open( e.fileName() ) ) {
                    qWarning() << e.fileName() <<  QChar(124).toLatin1()
                               << tr("Not a valid media file.");
                    continue;
                }
                VideoPicture *p = mediafile.getFirstFrame();

                if (!p ) {
                    qWarning() << e.fileName() <<  QChar(124).toLatin1()
                               << tr("Not a valid image file.");
                    continue;
                }

                // set rendering area
                QRect coords(index.x()*_elementsSize.width(),
                             index.y()*_elementsSize.height(),
                             _elementsSize.width(),
                             _elementsSize.height());
                glViewport(coords.x(), coords.y(), coords.width(), coords.height() );

                // set element coordinates of image in FBO and add it to list
                e.setCoordinates(coords);
                e.setPage(atlaspage);
                _atlasElements.append(e);

                // render image in FBO atlas page
                GLenum format = (p->getFormat() == AV_PIX_FMT_RGBA) ? GL_RGBA : GL_RGB;
                glPixelStorei(GL_UNPACK_ROW_LENGTH, p->getRowLength());
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p->getWidth(),
                             p->getHeight(), 0, format, GL_UNSIGNED_BYTE, p->getBuffer());
                glCallList(ViewRenderWidget::quad_texured);

                // next index
                if (index.x() < atlaspage->array().width())
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

            atlaspage->fbo()->release();
        }

        // decrement number of images to insert
        c -= atlaspage->count();
    }
    while (c > 0); // loop while more elements to insert

    // remove texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &tex);

    // ok if some files were added to the atlas
    return (numElementsAtlas < _atlasElements.count());
}


ImageAtlasPage::~ImageAtlasPage(){

    delete _fbo;
}

QRectF ImageAtlasPage::texturecoordinates(QRect rect) const {
    QRectF textcoords;

    double w = _fbo->width();
    double h = _fbo->height();
    textcoords.setX( (double) rect.x() / w);
    textcoords.setWidth( (double) rect.width() / w);
    textcoords.setY( (double) rect.y() / h);
    textcoords.setHeight( (double) rect.height() / h);

    return textcoords;
}

int ImageAtlasPage::texturesize() const {

    return _fbo->size().width() * _fbo->size().height() * 4;
}


int ImageAtlas::texturesize() const {

    int s = 0;
    foreach(ImageAtlasPage *P, _atlasPages) {
        s += P->texturesize();
    }
    return s;
}

ImageAtlasPage::ImageAtlasPage(QSize imagesize, int numimages){

    // Check limits of the openGL frame buffer dimensions
    GLint maxwidth = glMaximumFramebufferWidth();;
    GLint maxheight = glMaximumFramebufferHeight();

    _array.setWidth( qMin( numimages, maxwidth / imagesize.width() ) );
    _array.setHeight( qMin( numimages / _array.width(), maxheight / imagesize.height() ) );

    _fbo = new QGLFramebufferObject(_array.width() * imagesize.width(), _array.height() * imagesize.height());
    CHECK_PTR_EXCEPTION(_fbo);

}

