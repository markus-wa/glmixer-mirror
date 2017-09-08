#include "common.h"

#include <QFontDatabase>

QMap<int, QPair<int, int> > presetBlending;
QStringList listofextensions;

QString monospaceFont = "Courier";

// we will need these for the correspondence between comboBox and GLenums:
GLenum blendfunctionFromInt(int i){
    switch (i) {
    case 0:
        return GL_ZERO;
    case 1:
        return GL_ONE;
    case 2:
        return GL_SRC_COLOR;
    case 3:
        return GL_ONE_MINUS_SRC_COLOR;
    case 4:
        return GL_DST_COLOR;
    case 5:
        return GL_ONE_MINUS_DST_COLOR;
    case 6:
        return GL_SRC_ALPHA;
    default:
    case 7:
        return GL_ONE_MINUS_SRC_ALPHA;
    case 8:
        return GL_DST_ALPHA;
    case 9:
        return GL_ONE_MINUS_DST_ALPHA;
    }
}

int intFromBlendfunction(GLenum e){
    switch (e) {
    case GL_ZERO:
        return 0;
    case GL_ONE:
        return 1;
    case GL_SRC_COLOR:
        return 2;
    case GL_ONE_MINUS_SRC_COLOR:
        return 3;
    case GL_DST_COLOR:
        return 4;
    case GL_ONE_MINUS_DST_COLOR:
        return 5;
    case GL_SRC_ALPHA:
        return 6;
    default:
    case GL_ONE_MINUS_SRC_ALPHA:
        return 7;
    case GL_DST_ALPHA:
        return 8;
    case GL_ONE_MINUS_DST_ALPHA:
        return 9;
    }
}

GLenum blendequationFromInt(int i){
    switch (i) {
    case 0:
        return GL_FUNC_ADD;
    case 1:
        return GL_FUNC_SUBTRACT;
    case 2:
        return GL_FUNC_REVERSE_SUBTRACT;
    case 3:
        return GL_MIN;
    case 4:
        return GL_MAX;
    }
    return 0;
}

int intFromBlendequation(GLenum e){
    switch (e) {
    case GL_FUNC_ADD:
        return 0;
    case GL_FUNC_SUBTRACT:
        return 1;
    case GL_FUNC_REVERSE_SUBTRACT:
        return 2;
    case GL_MIN:
        return 3;
    case GL_MAX:
        return 4;
    }
    return 0;
}

void initListOfExtension()
{
    //
    // fill in the list of extensions by creating a dummy glwidget
    //
    QGLWidget *glw = new QGLWidget();
    glw->makeCurrent();
    glewInit();
    QString allextensions = QString( (char *) glGetString(GL_EXTENSIONS));
    listofextensions = allextensions.split(" ", QString::SkipEmptyParts);
    delete glw;

    //
    // Init blending preset list
    //
    // color mix
    presetBlending[1] = qMakePair( intFromBlendfunction(GL_ONE), intFromBlendequation(GL_FUNC_ADD) );
    // inverse color mix
    presetBlending[2] = qMakePair( intFromBlendfunction(GL_ONE), intFromBlendequation(GL_FUNC_REVERSE_SUBTRACT) );
    // layer color mix
    presetBlending[3] = qMakePair( intFromBlendfunction(GL_DST_COLOR), intFromBlendequation(GL_FUNC_ADD) );
    // layer inverse color mix
    presetBlending[4] = qMakePair( intFromBlendfunction(GL_DST_COLOR), intFromBlendequation(GL_FUNC_REVERSE_SUBTRACT) );
    // alpha blending
    presetBlending[5] = qMakePair( intFromBlendfunction(GL_ONE_MINUS_SRC_ALPHA), intFromBlendequation(GL_FUNC_ADD) );

}

QStringList glSupportedExtensions()
{
    return listofextensions;
}


GLint glMaximumTextureWidth()
{
    GLint max = 0;

    if (glewIsSupported("GL_ARB_internalformat_query2"))
        glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_MAX_WIDTH, 1, &max);
    else
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);

    return max;
}

GLint glMaximumTextureHeight()
{
    GLint max = 0;

    if (glewIsSupported("GL_ARB_internalformat_query2"))
        glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_MAX_HEIGHT, 1, &max);
    else
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);

    return max;
}

GLint glMaximumFramebufferWidth()
{
    GLint max = 0;

    if (glewIsSupported("GL_ARB_framebuffer_no_attachments"))
        glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &max);
    else
        max = glMaximumTextureWidth();

    return max;
}

GLint glMaximumFramebufferHeight()
{
    GLint max = 0;

    if (glewIsSupported("GL_ARB_framebuffer_no_attachments"))
        glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &max);
    else
        max = glMaximumTextureHeight();

    return max;
}

QPair<int, int> blendingPresetFromInt(int i)
{
    if (presetBlending.count(i) > 0)
        return presetBlending[i];
    else
        return presetBlending[1];
}

int intFromBlendingPreset(GLenum blendfunction, GLenum blendequation) {
    return presetBlending.key( qMakePair( intFromBlendfunction(blendfunction), intFromBlendequation(blendequation) ), 0 );
}

QString namePresetFromInt(int i)
{
    switch (i) {
    case 0:
        return QString("Custom");
    case 1:
        return QString("Add");
    case 2:
        return QString("Subtract");
    case 3:
        return QString("Layer add");
    case 4:
        return QString("Layer subtract");
    case 5:
        return QString("Opacity");
    }
    return 0;
}


void addPathToSystemPath(QByteArray path)
{
#ifdef Q_OS_MAC
    const char* pathEnvironmentVariableName = "PATH";
    const char separatorEnvironmentVariable = ':';
#else
#ifdef Q_OS_LINUX
    const char* pathEnvironmentVariableName = "PATH";
    const char separatorEnvironmentVariable = ':';
#else
    const char* pathEnvironmentVariableName = "PATH";
    const char separatorEnvironmentVariable = ';';
#endif
#endif
    // read the environment variable for getting system path
    QByteArray pathEnvironmentVariable = qgetenv( pathEnvironmentVariableName );

    // does this path already exist in the environment variable ?
    if ( ! QString(pathEnvironmentVariable).split(separatorEnvironmentVariable).contains(path)  ) {
        // does not exist so add it
        pathEnvironmentVariable.append( separatorEnvironmentVariable );
        pathEnvironmentVariable.append( path );
        // set the extended environment variable back
        qputenv( pathEnvironmentVariableName, pathEnvironmentVariable);
    }
}


QString getByteSizeString(double numbytes)
{
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while(numbytes >= 1024.0 && i.hasNext())
     {
        unit = i.next();
        numbytes /= 1024.0;
    }
    return QString().setNum(numbytes,'f',1) + " " + unit;
}



void initApplicationFonts()
{
    int fontid = QFontDatabase::addApplicationFont(":/glmixer/fonts/Hack-Regular.ttf");
    fontid = QFontDatabase::addApplicationFont(":/glmixer/fonts/Hack-Bold.ttf");
    fontid = QFontDatabase::addApplicationFont(":/glmixer/fonts/Hack-RegularOblique.ttf");
    fontid = QFontDatabase::addApplicationFont(":/glmixer/fonts/Hack-BoldOblique.ttf");

    if ( fontid > -1 ) {
        monospaceFont = QFontDatabase::applicationFontFamilies(fontid)[0];
        qDebug() << "Loaded font " << monospaceFont << QFontDatabase ().styles( monospaceFont );
    }
}


QString getMonospaceFont()
{
    return monospaceFont;
}


#ifdef Q_OS_MAC
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFUrl.h>

QFileInfo getFileInfoFromURL(QUrl url)
{
    QString localFileQString = url.toLocalFile();
    // [pzion 20150805] Work around
    // https://bugreports.qt.io/browse/QTBUG-40449
    if ( localFileQString.startsWith("/.file/id=") )
    {
        CFStringRef relCFStringRef = CFStringCreateWithCString( kCFAllocatorDefault,
            localFileQString.toUtf8().constData(),
            kCFStringEncodingUTF8 );
        CFURLRef relCFURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault,
            relCFStringRef,
            kCFURLPOSIXPathStyle,
            false );
        CFErrorRef error = 0;
        CFURLRef absCFURL = CFURLCreateFilePathURL( kCFAllocatorDefault,
            relCFURL, &error );
        if ( !error ) {
            static const CFIndex maxAbsPathCStrBufLen = 4096;
            char absPathCStr[maxAbsPathCStrBufLen];
            if ( CFURLGetFileSystemRepresentation(
                absCFURL,
                true, // resolveAgainstBase
                reinterpret_cast<UInt8 *>( &absPathCStr[0] ),
                maxAbsPathCStrBufLen
                ) )
            {
                localFileQString = QString( absPathCStr );
            }
        }
        CFRelease( absCFURL );
        CFRelease( relCFURL );
        CFRelease( relCFStringRef );
    }

    return QFileInfo (localFileQString);
}

#else

QFileInfo getFileInfoFromURL(QUrl url)
{
    return QFileInfo(url.toLocalFile());
}
#endif
