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

#include <QObject>
#include <QGLFramebufferObject>

#include "ViewRenderWidget.h"
#include "SourceSet.h"

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
	void addSource();
	void addSource(VideoFile *vf);
#ifdef OPEN_CV
	void addSource(int opencvIndex);
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
	void clearSourceSet();
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


signals:
	void currentSourceChanged(SourceSet::iterator csi);

private:
	RenderingManager();
	virtual ~RenderingManager();
	static RenderingManager *_instance;

protected:
	ViewRenderWidget *_renderwidget;
	QGLFramebufferObject *_fbo;
	QGLFramebufferObject *previousframe_fbo;
	int countRenderingSource;
    static bool blit;

	// the set of sources
	SourceSet _sources;
	SourceSet::iterator currentSource;

};

#endif /* MAINRENDERWIDGET_H_ */
