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

	SourceSet::iterator getById(const GLuint id) const;
	SourceSet::iterator getByName(const QString name) const;
	bool notAtEnd(SourceSet::iterator itsource) const;
	bool isValid(SourceSet::iterator itsource) const;
	QString getAvailableNameFrom(QString name) const;
	double getAvailableDepthFrom(double depth = -1) const;
	SourceSet::iterator changeDepth(SourceSet::iterator itsource,double newdepth);
	inline SourceSet::iterator getBegin()  const{
		return _sources.begin();
	}
	inline SourceSet::iterator getEnd()  const{
		return _sources.end();
	}
	void removeSource(SourceSet::iterator itsource);
	bool setCurrentSource(SourceSet::iterator si);
	bool setCurrentSource(GLuint name);
	inline SourceSet::iterator getCurrentSource()  const{
		return _currentSource;
	}
	bool setCurrentNext();
	bool setCurrentPrevious();

	void addSourceToBasket(Source *s);
	int getSourceBasketSize() const;
	Source *getSourceBasketTop() const;

	/**
	 * management of the rendering
	 */
	void setFrameBufferResolution(QSize size);
	void renderToFrameBuffer(Source *source, bool clearfirst);
	float getFrameBufferAspectRatio() const;
	inline QSize getFrameBufferResolution() const {
			return _fbo ? _fbo->size() : QSize(0,0);
	}
	inline int getFrameBufferWidth() const{
		return _fbo ? _fbo->width() : 0;
	}
	inline int getFrameBufferHeight() const{
		return _fbo ? _fbo->height() : 0;
	}

	inline GLuint getFrameBufferTexture() const{
		return _fbo ? _fbo->texture() : 0;
	}
	inline GLuint getCatalogTexture() const{
		return _fbo ? _fboCatalogTexture : 0;
	}
	inline GLuint getFrameBufferHandle() const{
		return _fbo ? _fbo->handle() : 0;
	}

	void updatePreviousFrame();
	inline int getPreviousFrameDelay() const {
		return previousframe_delay;
	}

	QImage captureFrameBuffer();
	inline bool clearToWhite() const {
		return clearWhite;
	}

	/**
	 * save and load configuration
	 */
	QDomElement getConfiguration(QDomDocument &doc);
	void addConfiguration(QDomElement xmlconfig);

public Q_SLOTS:

	void setClearToWhite(bool on) { clearWhite = on; }
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
    bool clearWhite;

	// the set of sources
	SourceSet _sources;
	SourceSet::iterator _currentSource;
	SourceList dropBasket;

	// the defaults
	QMap<QString, QVariant> defaults;

    static bool blit_fbo_extension;
};

#endif /* MAINRENDERWIDGET_H_ */
