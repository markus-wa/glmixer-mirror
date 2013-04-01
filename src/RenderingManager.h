/*
 * RenderingManager
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2012 Bruno Herbelin
 *
 */

#ifndef RENDERINGMANAGER_H_
#define RENDERINGMANAGER_H_

//#define USE_GLREADPIXELS

#include "common.h"
#include "SourceSet.h"

typedef enum {
	QUALITY_VGA = 0,
	QUALITY_PAL,
	QUALITY_SVGA,
	QUALITY_XGA,
	QUALITY_UXGA,
	QUALITY_UNSUPPORTED
} frameBufferQuality;

typedef enum {
	ASPECT_RATIO_4_3 = 0,
	ASPECT_RATIO_3_2,
	ASPECT_RATIO_16_10,
	ASPECT_RATIO_16_9,
	ASPECT_RATIO_FREE
} standardAspectRatio;

standardAspectRatio doubleToAspectRatio(double ar);

#include <QObject>
#include <QDomElement>
#include <QtSvg>

class QGLFramebufferObject;
class VideoFile;
class ViewRenderWidget;
class SourcePropertyBrowser;
class RenderingEncoder;
class SessionSwitcher;

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
	static RenderingEncoder *getRecorder();
	static SessionSwitcher *getSessionSwitcher();
	static RenderingManager *getInstance();
	static void deleteInstance();

	/**
	* Management of the sources
	**/
	QString renameSource(Source *s, const QString name);

	// create source per type :
	Source *newRenderingSource(double depth = -1.0);
	Source *newCaptureSource(QImage img, double depth = -1.0);
	Source *newMediaSource(VideoFile *vf, double depth = -1.0);
#ifdef OPEN_CV
	Source *newOpencvSource(int opencvIndex, double depth = -1.0);
#endif
	Source *newSvgSource(QSvgRenderer *svg, double depth = -1.0);
	Source *newAlgorithmSource(int type, int w, int h, double v, int p, bool ia, double depth = -1.0);
	Source *newSharedMemorySource(qint64 shmid, double depth = -1.0);
	Source *newCloneSource(SourceSet::iterator sit, double depth = -1.0);
	// insert the source into the scene
	bool insertSource(Source *s);

    SourceSet::iterator getBegin();
    SourceSet::iterator getEnd();
    SourceSet::const_iterator getBegin() const;
    SourceSet::const_iterator getEnd() const;
    SourceSet::iterator getById(const GLuint id);
    SourceSet::iterator getByName(const QString name);
    SourceSet::const_iterator getByName(const QString name) const;
    bool notAtEnd(SourceSet::const_iterator itsource) const;
    bool isValid(SourceSet::const_iterator itsource) const;
	QString getAvailableNameFrom(QString name) const;
	double getAvailableDepthFrom(double depth = -1) const;
	SourceSet::iterator changeDepth(SourceSet::iterator itsource,double newdepth);

	inline bool empty() const { return _sources.empty(); }

	void removeSource(SourceSet::iterator itsource);
	void removeSource(const GLuint idsource);
	bool isCurrentSource(Source *s);
	bool isCurrentSource(SourceSet::iterator si);
	void setCurrentSource(SourceSet::iterator si);
	void setCurrentSource(GLuint id);
	inline SourceSet::iterator getCurrentSource()  const{
		return _currentSource;
	}
	bool setCurrentNext();
	bool setCurrentPrevious();
	void unsetCurrentSource() { setCurrentSource( getEnd() ); }


	void addSourceToBasket(Source *s);
	int getSourceBasketSize() const;
	Source *getSourceBasketTop() const;

	/**
	 * management of the rendering
	 */
	void setRenderingQuality(frameBufferQuality q);
	inline frameBufferQuality getRenderingQuality() const {
		return renderingQuality;
	}

	void setRenderingAspectRatio(standardAspectRatio ar);
	standardAspectRatio getRenderingAspectRatio() const {
		return renderingAspectRatio;
	}

	void setGammaShift(float g);
	float getGammaShift() const;

	double getFrameBufferAspectRatio() const;
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

	void renderToFrameBuffer(Source *source, bool first, bool last = false);
	void postRenderToFrameBuffer();
	inline unsigned int getPreviousFrameDelay() const {
		return previousframe_delay;
	}

	QImage captureFrameBuffer(QImage::Format format = QImage::Format_RGB888);
	inline bool clearToWhite() const {
		return clearWhite;
	}

	uint getSharedMemoryColorDepth();
	void setSharedMemoryColorDepth(uint mode);

	/**
	 * save and load configuration
	 */
	QDomElement getConfiguration(QDomDocument &doc, QDir current);
	int addConfiguration(QDomElement xmlconfig, QDir current);
	inline Source *defaultSource() { return _defaultSource; }
	inline Source::scalingMode getDefaultScalingMode() const { return _scalingMode; }
	inline void setDefaultScalingMode(Source::scalingMode sm) { _scalingMode = sm; }
	inline bool getDefaultPlayOnDrop() const { return _playOnDrop; }
	inline void setDefaultPlayOnDrop(bool on){ _playOnDrop = on; }
	inline bool isPaused () { return paused; }

	static bool getUseFboBlitExtension() { return blit_fbo_extension; }
	static void setUseFboBlitExtension(bool on);

public Q_SLOTS:

	void setClearToWhite(bool on) { clearWhite = on; }
	void setPreviousFrameDelay(unsigned int delay) { previousframe_delay = CLAMP(delay,1,1000);}

	void pause(bool on);
	void clearBasket();
	void clearSourceSet();
	void resetSource(SourceSet::iterator sit);
	void resetCurrentSource();
	void startCurrentSource(bool on);
	void toggleMofifiableCurrentSource();

	bool dropSource();
	void dropSourceWithAlpha(double alphax, double alphay);
	void dropSourceWithCoordinates(double x, double y);
	void dropSourceWithDepth(double depth);
	void disableProgressBars(bool off) { _showProgressBar = !off; }

	void setFrameSharingEnabled(bool on);

Q_SIGNALS:
	void currentSourceChanged(SourceSet::iterator csi);

private:
	RenderingManager();
	virtual ~RenderingManager();
	static RenderingManager *_instance;

	void setFrameBufferResolution(QSize size);

protected:
	// the rendering area
	ViewRenderWidget *_renderwidget;
    class SourcePropertyBrowser *_propertyBrowser;

	// the frame buffer
	QGLFramebufferObject *_fbo;
	GLuint _fboCatalogTexture;
	QGLFramebufferObject *previousframe_fbo;
	unsigned int countRenderingSource, previousframe_index, previousframe_delay;
    bool clearWhite;
    frameBufferQuality renderingQuality;
    standardAspectRatio renderingAspectRatio;
    float gammaShift;

    // The shared memory buffer
    class QSharedMemory *_sharedMemory;
    GLenum _sharedMemoryGLFormat, _sharedMemoryGLType;

	// the set of sources
	SourceSet _sources;
    // the recorder
    RenderingEncoder *_recorder;
    // the session switcher
    SessionSwitcher *_switcher;

	// manipulation of sources
	SourceSet::iterator _currentSource;
	SourceList dropBasket;
	Source *_defaultSource;
	Source::scalingMode _scalingMode;
	bool _playOnDrop;
	bool paused;
	bool _showProgressBar;

    static bool blit_fbo_extension;
    static QSize sizeOfFrameBuffer[ASPECT_RATIO_FREE][QUALITY_UNSUPPORTED];
};

#endif /* RENDERINGMANAGER_H_ */
