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

#include <QWidget>
#include <QGLFramebufferObject>


#include "SourceSet.h"

class VideoFile;
class RenderWidget;

class RenderingManager: public QObject {

Q_OBJECT

	friend class RenderWidget;
	friend class Source;
	friend class MixerView;
	friend class GeometryView;
	friend class OutputRenderWindow;

public:
	/**
	 * singleton mechanism
	 */
	static QGLWidget *getQGLWidget();
	static RenderingManager *getInstance();

	/**
	* Management of the sources
	**/
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
	 * management of the manipulation views
	 */
	typedef enum {NONE = 0, MIXING=1, GEOMETRY=2, DEPTH=3 } viewMode;
	void setViewMode(viewMode mode);
	QPixmap getViewIcon();

	/**
	 * management of the rendering
	 */
	void setFrameBufferResolution(int width, int height);
	void bindFrameBuffer();
	void releaseFrameBuffer();
	GLuint getFrameBufferTexture();
	float getFrameBufferAspectRatio();
	int getFrameBufferWidth();
	int getFrameBufferHeight();


public slots:
	void zoomIn();
	void zoomOut();
	void zoomReset();
	void zoomBestFit();

signals:
	void currentSourceChanged(SourceSet::iterator csi);

private:
	RenderingManager();
	virtual ~RenderingManager();
	static RenderingManager *_instance;

protected:
	RenderWidget *_renderwidget;
	QGLFramebufferObject *_fbo;

	// the set of sources
	SourceSet _sources;
	SourceSet::iterator currentSource;

	// all the display lists
	static GLuint border_thin_shadow, border_large_shadow;
	static GLuint border_thin, border_large, border_scale;
	static GLuint frame_selection, frame_screen;
	static GLuint quad_texured, quad_half_textured, quad_black;
	static GLuint circle_mixing;

};

#endif /* MAINRENDERWIDGET_H_ */
