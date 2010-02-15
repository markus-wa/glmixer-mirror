/*
 * RenderingManager
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef MAINRENDERWIDGET_H_
#define MAINRENDERWIDGET_H_

#define DEFAULT_WIDTH 1024
#define DEFAULT_HEIGHT 768

#include "common.h"
#include "ViewRenderWidget.h"
#include "SourceSet.h"

#include <QObject>

class QGLFramebufferObject;

class VideoFile;

class RenderingManager: public QObject {

	Q_OBJECT

	friend class RenderingSource;
	friend class OutputRenderWidget;

public:
	/**
	 * singleton mechanism
	 */
	static ViewRenderWidget *getRenderingWidget();
	static RenderingManager *getInstance();

	/**
	* Management of the sources
	**/
	void addRenderingSource();
	void addCaptureSource();
	void addMediaSource(VideoFile *vf);
#ifdef OPEN_CV
	void addOpencvSource(int opencvIndex);
#endif
	inline Source *getSource(int i) {
		return *_sources.begin();
	}
	SourceSet::iterator getById(GLuint name);
	bool notAtEnd(SourceSet::iterator itsource);
	bool isValid(SourceSet::iterator itsource);
	SourceSet::iterator changeDepth(SourceSet::iterator itsource,double newdepth);
	inline SourceSet::iterator getBegin() {
		return _sources.begin();
	}
	inline SourceSet::iterator getEnd() {
		return _sources.end();
	}
	void removeSource(SourceSet::iterator itsource);
	void setCurrentSource(SourceSet::iterator si);
	void setCurrentSource(GLuint name);
	inline SourceSet::iterator getCurrentSource() {
		return currentSource;
	}

	/**
	 * management of the rendering
	 */
	void setFrameBufferResolution(int width, int height);
	void renderToFrameBuffer(SourceSet::iterator itsource, bool clearfirst);
	GLuint getFrameBufferTexture();
	GLuint getFrameBufferHandle();
	float getFrameBufferAspectRatio();
	int getFrameBufferWidth();
	int getFrameBufferHeight();
	void updatePreviousFrame();
	int getPreviousFrameDelay() { return previousframe_delay; }


public slots:
	void setPreviousFrameDelay(int delay) { previousframe_delay = CLAMP(delay,1,1000);}
	void clearSourceSet();
	void captureFrameBuffer();
	void saveCapturedFrameBuffer(QString filename);

signals:
	void currentSourceChanged(SourceSet::iterator csi);

private:
	RenderingManager();
	virtual ~RenderingManager();
	static RenderingManager *_instance;

protected:
	// the rendering area
	ViewRenderWidget *_renderwidget;

	// the frame buffer
	QGLFramebufferObject *_fbo;
	QGLFramebufferObject *previousframe_fbo;
	int countRenderingSource, previousframe_index, previousframe_delay;
    static bool blit;
    QImage capture;

	// the set of sources
	SourceSet _sources;
	SourceSet::iterator currentSource;

};

#endif /* MAINRENDERWIDGET_H_ */
