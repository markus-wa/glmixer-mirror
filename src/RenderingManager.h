/*
 * RenderingManager
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#ifndef MAINRENDERWIDGET_H_
#define MAINRENDERWIDGET_H_

#define DEFAULT_WIDTH 960
#define DEFAULT_HEIGHT 600

#include "common.h"
#include "SourceSet.h"

class ViewRenderWidget;
class SourcePropertyBrowser;

#include <QObject>
#include <QDomElement>

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
	Source *addRenderingSource(double depth = -1.0);
	Source *addCaptureSource(QImage img, double depth = -1.0);
	Source *addMediaSource(VideoFile *vf, double depth = -1.0);
#ifdef OPEN_CV
	Source *addOpencvSource(int opencvIndex, double depth = -1.0);
#endif
	Source *addAlgorithmSource(int type, int w, int h, double v, int p, double depth = -1.0);
	Source *addCloneSource(SourceSet::iterator sit, double depth = -1.0);


	SourceSet::iterator getById(GLuint id);
	SourceSet::iterator getByName(QString name);
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

	/**
	 * save and load configuration
	 */
	QDomElement getConfiguration(QDomDocument &doc);
	void addConfiguration(QDomElement xmlconfig);

public Q_SLOTS:
	void setPreviousFrameDelay(int delay) { previousframe_delay = CLAMP(delay,1,1000);}
	void clearSourceSet();

Q_SIGNALS:
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
