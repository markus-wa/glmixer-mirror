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

#include <QWidget>

#include "VideoSource.h"
#include "SourceSet.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

class RenderWidget;

class MainRenderWidget: public QWidget {

Q_OBJECT

	friend class RenderWidget;

public:
	// get singleton instance
	static const QGLWidget *getQGLWidget();
	static MainRenderWidget *getInstance();
	static void deleteInstance();

	/**
	* Management of the video sources
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
	 * management of the rendering
	 */
	virtual void keyPressEvent(QKeyEvent * event);
	virtual void mouseDoubleClickEvent(QMouseEvent * event);
	virtual void closeEvent(QCloseEvent * event);

	/**
	 * Interaction with the other views
	 */
	void setRenderingResolution(int width, int height);
	float getRenderingAspectRatio();

	static GLuint border_thin_shadow, border_large_shadow;
	static GLuint border_thin, border_large, border_scale;
	static GLuint frame_selection, frame_screen;
	static GLuint quad_texured, quad_half_textured, quad_black;
	static GLuint circle_mixing;

public slots:
	void useRenderingAspectRatio(bool on);
	void setFullScreen(bool on);

signals:
	void currentSourceChanged(SourceSet::iterator csi);
	void windowClosed();

	/**
	 * singleton mechanism
	 */
private:
	MainRenderWidget();
	virtual ~MainRenderWidget();
	static MainRenderWidget *_instance;

protected:
	RenderWidget *_renderwidget;

	// the set of sources
	SourceSet _sources;
	SourceSet::iterator currentSource;

};

#endif /* MAINRENDERWIDGET_H_ */
