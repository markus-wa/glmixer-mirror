#include "ImageAtlas.moc"

#include "common.h"
#include "ViewRenderWidget.h"

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
        return true;

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

                // set rendering area
                QRect coords(index.x()*_elementsSize.width(),
                             index.y()*_elementsSize.height(),
                             _elementsSize.width(),
                             _elementsSize.height());
                glViewport(coords.x(), coords.y(), coords.width(), coords.height() );

                // create element with filename and coordinates of image in FBO
                ImageAtlasElement e(files.takeFirst());
                e.setCoordinates(coords);
                e.setPage(atlaspage);
                _atlasElements.append(e);

                // render image in FBO atlas page
                QImage image(e.fileName());
                if (image.format() != QImage::Format_ARGB32_Premultiplied)
                    image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    #if QT_VERSION >= 0x040700
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                             image.width(), image. height(),
                             0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.constBits() );
    #else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                             image.width(), image. height(),
                             0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.bits() );
    #endif
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

    // success if all files were added
    return (files.empty());
}


ImageAtlasPage::~ImageAtlasPage(){

    delete _fbo;
}

ImageAtlasPage::ImageAtlasPage(QSize imagesize, int numimages){

    // Check limits of the openGL frame buffer dimensions
    GLint maxwidth = 0;
    GLint maxheight = 0;
    if (glewIsSupported("GL_ARB_framebuffer_no_attachments")) {
        glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &maxwidth);
        glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &maxheight);
    }
    // if cannot access this extension, use safe value
    else
    {
        maxwidth = glMaximumTextureWidth();
        maxheight = glMaximumTextureHeight();
    }

    _array.setWidth( qMin( numimages, maxwidth / imagesize.width() ) );
    _array.setHeight( qMin( numimages / _array.width(), maxheight / imagesize.height() ) );

    _fbo = new QGLFramebufferObject(_array.width() * imagesize.width(), _array.height() * imagesize.height());
    CHECK_PTR_EXCEPTION(_fbo);

}

