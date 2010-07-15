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
	// create source per type :
	Source *newRenderingSource(double depth = -1.0);
	Source *newCaptureSource(QImage img, double depth = -1.0);
	Source *newMediaSource(VideoFile *vf, double depth = -1.0);
#ifdef OPEN_CV
	Source *newOpencvSource(int opencvIndex, double depth = -1.0);
#endif
	Source *newAlgorithmSource(int type, int w, int h, double v, int p, double depth = -1.0);
	Source *newCloneSource(SourceSet::iterator sit, double depth = -1.0);
	// insert the source into the scene
	void insertSource(Source *s);

	SourceSet::iterator getById(GLuint id);
	SourceSet::iterator getByName(QString name);
	bool notAtEnd(SourceSet::iterator itsource);
	bool isValid(SourceSet::iterator itsource);
	QString getAvailableNameFrom(QString name);
	double getAvailableDepthFrom(double depth = -1);
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
	bool setCurrentNext();
	bool setCurrentPrevious();

	void addSourceToBasket(Source *s);
	int getSourceBasketSize();
	Source *getSourceBasketTop();

	/**
	 * management of the rendering
	 */
	void setFrameBufferResolution(int width, int height);
	void renderToFrameBuffer(Source *source, bool clearfirst);
	GLuint getFrameBufferTexture();
	GLuint getCatalogTexture();
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

	void setClearColor(QColor c) { clearColor = c; }
	void setPreviousFrameDelay(int delay) { previousframe_delay = CLAMP(delay,1,1000);}

	void clearSourceSet();

	void dropSourceWithAlpha(double alphax, double alphay);
	void dropSourceWithCoordinates(double x, double y);
	void dropSourceWithDepth(double depth);

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
	GLuint _fboCatalogTexture;
	QGLFramebufferObject *previousframe_fbo;
	int countRenderingSource, previousframe_index, previousframe_delay;
    QColor clearColor;

	// the set of sources
	SourceSet _sources;
	SourceSet::iterator _currentSource;
	SourceList dropBasket;

    static bool blit_fbo_extension;
};

#endif /* MAINRENDERWIDGET_H_ */
