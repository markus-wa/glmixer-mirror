/*
 * MixRenderWidget.h
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef MAINRENDERWIDGET_H_
#define MAINRENDERWIDGET_H_

#define DEFAULT_WIDTH 1024
#define DEFAULT_HEIGHT 768

#include "glRenderWidget.h"
#include "VideoSource.h"
#include "SourceSet.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

class MainRenderWidget: public glRenderWidget {

	Q_OBJECT

public:
    // get singleton instance
    static const QGLWidget *getQGLWidget();
    static MainRenderWidget *getInstance();
    static void deleteInstance();


    // QGLWidget rendering
    void paintGL();

    // Management of the video sources
    void addSource(VideoFile *vf);
#ifdef OPEN_CV
    void addSource(int opencvIndex);
#endif
    inline Source *getSource(int i) { return *_sources.begin();}
    SourceSet::iterator getById(GLuint name);
    bool notAtEnd(SourceSet::iterator itsource);
    bool isValid(SourceSet::iterator itsource);
    SourceSet::iterator changeDepth(SourceSet::iterator itsource, double newdepth);
    inline SourceSet::iterator getBegin() { return _sources.begin(); }
    inline SourceSet::iterator getEnd() { return _sources.end(); }
    void removeSource(SourceSet::iterator itsource);
    void clearSourceSet();

    void setCurrentSource(SourceSet::iterator si);
    inline SourceSet::iterator getCurrentSource() { return currentSource; }

    // management of the rendering
    void setRenderingResolution(int w, int h);
    float getRenderingAspectRatio() {return _aspectRatio;}

public slots:
    void useRenderingAspectRatio(bool on) {_useAspectRatio = on;}

signals:
	void currentSourceChanged(SourceSet::iterator csi);

    // singleton mechanism
private:
	MainRenderWidget(QWidget *parent = 0);
	virtual ~MainRenderWidget();
    static MainRenderWidget *_instance;

private:
    // the set of sources
    SourceSet _sources;
    SourceSet::iterator currentSource;

    // TODO: implement the use of fbo
    QGLFramebufferObject *_fbo;

	float _aspectRatio;
	bool _useAspectRatio;

};

#endif /* MAINRENDERWIDGET_H_ */
