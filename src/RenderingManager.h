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
#include "SourceSet.h"

class ViewRenderWidget;
class SourcePropertyBrowser;

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
	static SourcePropertyBrowser *getPropertyBrowserWidget();
	static RenderingManager *getInstance();
	static void deleteInstance();

	/**
	* Management of the sources
	**/
	void addRenderingSource();
	void addCaptureSource(QImage img);
	void addMediaSource(VideoFile *vf);
#ifdef OPEN_CV
	void addOpencvSource(int opencvIndex);
#endif
	void addAlgorithmSource(int type, int w, int h, double v, int p);
	void addCloneSource(SourceSet::iterator sit);


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
	bool setCurrentSource(SourceSet::iterator si);
	bool setCurrentSource(GLuint name);
	inline SourceSet::iterator getCurrentSource() {
		return _currentSource;
	}

	/**
	 * management of the rendering
	 */
	void setFrameBufferResolution(int width, int height);
	void clearFrameBuffer();
	void renderToFrameBuffer(SourceSet::iterator itsource, bool clearfirst);
	GLuint getFrameBufferTexture();
	GLuint getFrameBufferHandle();
	float getFrameBufferAspectRatio();
	int getFrameBufferWidth();
	int getFrameBufferHeight();
	void updatePreviousFrame();
	int getPreviousFrameDelay() { return previousframe_delay; }

	QImage captureFrameBuffer();

public slots:
	void setPreviousFrameDelay(int delay) { previousframe_delay = CLAMP(delay,1,1000);}
	void clearSourceSet();

signals:
	void currentSourceChanged(SourceSet::iterator csi);

private:
	RenderingManager();
	virtual ~RenderingManager();
	static RenderingManager *_instance;

protected:
	// the rendering area
	ViewRenderWidget *_renderwidget;
    class SourcePropertyBrowser *_propertyBrowser;

	// the frame buffer
	QGLFramebufferObject *_fbo;
	QGLFramebufferObject *previousframe_fbo;
	int countRenderingSource, previousframe_index, previousframe_delay;
    static bool blit;
    QColor clearColor;

	// the set of sources
	SourceSet _sources;
	SourceSet::iterator _currentSource;

};

#endif /* MAINRENDERWIDGET_H_ */
