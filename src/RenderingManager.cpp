/*
 * RenderingManager.cpp
 *
 *  Created on: 3 nov. 2009
 *      Author: herbelin
 */

#include "RenderingManager.moc"

#include "common.h"

#include "RenderingSource.h"
Source::RTTI RenderingSource::type = Source::RENDERING_SOURCE;
#include "CloneSource.h"
Source::RTTI CloneSource::type = Source::CLONE_SOURCE;
#include "CaptureSource.h"
Source::RTTI CaptureSource::type = Source::CAPTURE_SOURCE;

#include "ViewRenderWidget.h"
#include "SourcePropertyBrowser.h"
#include "AlgorithmSource.h"
#include "VideoFile.h"
#include "VideoSource.h"
#ifdef OPEN_CV
#include "OpencvSource.h"
#endif

#include <algorithm>
#include <QGLFramebufferObject>

// static members
RenderingManager *RenderingManager::_instance = 0;
bool RenderingManager::blit_fbo_extension = false;

ViewRenderWidget *RenderingManager::getRenderingWidget() {

	return getInstance()->_renderwidget;
}

SourcePropertyBrowser *RenderingManager::getPropertyBrowserWidget() {

	return getInstance()->_propertyBrowser;
}

RenderingManager *RenderingManager::getInstance() {

	if (_instance == 0) {

		if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
			qCritical( "*** ERROR ***\n\nOpenGL Frame Buffer Objects are not supported on this graphics hardware."
					"\n\nThe program cannot operate properly.\n\nExiting...");

		if (glSupportsExtension("GL_EXT_framebuffer_blit"))
			RenderingManager::blit_fbo_extension = true;
		else
			qWarning( "** WARNING **\n\nOpenGL extension GL_EXT_framebuffer_blit is not supported on this graphics hardware."
					"\n\nRendering speed be sub-optimal but all should work properly.");

		if (!glSupportsExtension("GL_ARB_imaging")) {
			Source::imaging_extension = false;
			qWarning( "** WARNING **\n\nOpenGL extension GL_ARB_imaging is not supported on this graphics hardware."
					"\n\nSeveral image processing operations will not work (brigthness, contrast, filters, etc.).");
		}


		_instance = new RenderingManager;
		Q_CHECK_PTR(_instance);
	}

	return _instance;
}

void RenderingManager::deleteInstance() {
	if (_instance != 0)
		delete _instance;
}

RenderingManager::RenderingManager() :
	QObject(), _fbo(NULL), previousframe_fbo(NULL), countRenderingSource(0),
			previousframe_index(0), previousframe_delay(1) {

	_renderwidget = new ViewRenderWidget;
	Q_CHECK_PTR(_renderwidget);

	_propertyBrowser = new SourcePropertyBrowser;
	Q_CHECK_PTR(_propertyBrowser);

    QObject::connect(this, SIGNAL(currentSourceChanged(SourceSet::iterator)), _propertyBrowser, SLOT(showProperties(SourceSet::iterator) ) );

    QObject::connect(_renderwidget, SIGNAL(sourceMixingModified()), _propertyBrowser, SLOT(updateMixingProperties() ) );
    QObject::connect(_renderwidget, SIGNAL(sourceGeometryModified()), _propertyBrowser, SLOT(updateGeometryProperties() ) );
    QObject::connect(_renderwidget, SIGNAL(sourceLayerModified()), _propertyBrowser, SLOT(updateLayerProperties() ) );

    QObject::connect(_renderwidget, SIGNAL(sourceMixingDrop(double,double)), this, SLOT(dropSourceWithAlpha(double, double) ) );
    QObject::connect(_renderwidget, SIGNAL(sourceGeometryDrop(double,double)), this, SLOT(dropSourceWithCoordinates(double, double)) );
    QObject::connect(_renderwidget, SIGNAL(sourceLayerDrop(double)), this, SLOT(dropSourceWithDepth(double)) );

	setFrameBufferResolution(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	_currentSource = getEnd();

	clearColor = QColor(Qt::black);
}

RenderingManager::~RenderingManager() {

	clearSourceSet();

	if (_renderwidget != 0)
		delete _renderwidget;

	if (_fbo)
		delete _fbo;

}

void RenderingManager::setFrameBufferResolution(int width, int height) {

	if (_fbo)
		delete _fbo;

	_renderwidget->makeCurrent();
	_fbo = new QGLFramebufferObject(width, height);
	Q_CHECK_PTR(_fbo);

}

float RenderingManager::getFrameBufferAspectRatio() {

	return ((float) _fbo->width() / (float) _fbo->height());
}

GLuint RenderingManager::getFrameBufferTexture() {
	return _fbo->texture();
}
GLuint RenderingManager::getFrameBufferHandle() {
	return _fbo->handle();
}
int RenderingManager::getFrameBufferWidth() {
	return _fbo->width();
}
int RenderingManager::getFrameBufferHeight() {
	return _fbo->height();
}

void RenderingManager::updatePreviousFrame() {

	if (!previousframe_fbo)
		return;

	// TODO: implement the GUI for selecting frame delay
//	previousframe_index++;
//
//	if (previousframe_index % previousframe_delay)
//		return;
//	else
//		previousframe_index = 0;

	if (RenderingManager::blit_fbo_extension)
	// use the accelerated GL_EXT_framebuffer_blit if available
	{
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, _fbo->handle());

		// TODO : Can we draw in different texture buffer so we can keep an history of
		// several frames, and each loopback source could use a different one
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, previousframe_fbo->handle());

		glBlitFramebufferEXT(0, _fbo->height(), _fbo->width(), 0, 0, 0,
				previousframe_fbo->width(), previousframe_fbo->height(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	} else
	// 	Draw quad with fbo texture in a more basic OpenGL way
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);

		glViewport(0, 0, previousframe_fbo->width(),
				previousframe_fbo->height());

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glColor4f(1.0, 1.0, 1.0, 1.0);

		// render to the frame buffer object
		previousframe_fbo->bind();
		glBindTexture(GL_TEXTURE_2D, _fbo->texture());

		glCallList(ViewRenderWidget::quad_texured);
		previousframe_fbo->release();

		// pop the projection matrix and GL state back for rendering the current view
		// to the actual widget
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

}

void RenderingManager::clearFrameBuffer() {
	_fbo->bind();
	{
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glClearColor(clearColor.redF(), clearColor.greenF(),
				clearColor.blueF(), 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glPopAttrib();
	}
	_fbo->release();
}

void RenderingManager::renderToFrameBuffer(SourceSet::iterator itsource,
		bool clearfirst) {

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

	glViewport(0, 0, _fbo->width(), _fbo->height());

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(-SOURCE_UNIT, SOURCE_UNIT, -SOURCE_UNIT, SOURCE_UNIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// render to the frame buffer object
	_fbo->bind();
	{
		if (clearfirst) {
			glClearColor(clearColor.redF(), clearColor.greenF(),
					clearColor.blueF(), 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		if (!(*itsource)->isCulled() && (*itsource)->getAlpha() > 0.0) {
			glTranslated((*itsource)->getX(), (*itsource)->getY(), 0.0);
			glScaled((*itsource)->getScaleX(), (*itsource)->getScaleY(), 1.f);

			(*itsource)->blend();
			(*itsource)->draw();
		}
	}
	_fbo->release();

	// pop the projection matrix and GL state back for rendering the current view
	// to the actual widget
	glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

Source *RenderingManager::newRenderingSource(double depth) {

	_renderwidget->makeCurrent();
	// create the previous frame (frame buffer object) if needed
	if (!previousframe_fbo) {
		previousframe_fbo = new QGLFramebufferObject(_fbo->width(),
				_fbo->height());
	}

	// create a source appropriate for this videofile
	RenderingSource *s = new RenderingSource(previousframe_fbo->texture(), getAvailableDepthFrom(depth));
	Q_CHECK_PTR(s);

	return ( (Source *) s );
}

QImage RenderingManager::captureFrameBuffer() {

	_renderwidget->makeCurrent();
	return _fbo->toImage();
}

Source *RenderingManager::newCaptureSource(QImage img, double depth) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	// high priority means low variability
	GLclampf highpriority = 1.0;
	glPrioritizeTextures(1, &textureIndex, &highpriority);

	// create a source appropriate for this videofile
	CaptureSource *s = new CaptureSource(img, textureIndex, getAvailableDepthFrom(depth));
	Q_CHECK_PTR(s);

	return ( (Source *) s );
}

Source *RenderingManager::newMediaSource(VideoFile *vf, double depth) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	// low priority means high variability
	GLclampf lowpriority = 0.1;
	glPrioritizeTextures(1, &textureIndex, &lowpriority);

	// create a source appropriate for this videofile
	VideoSource *s = new VideoSource(vf, textureIndex, getAvailableDepthFrom(depth) );
	Q_CHECK_PTR(s);

	// scale the source to match the media size
	s->resetScale();

	return ( (Source *) s );
}

#ifdef OPEN_CV
Source *RenderingManager::newOpencvSource(int opencvIndex, double depth) {

	GLuint textureIndex;
	OpencvSource *s = 0;

	// try to create the OpenCV source
	try {
		// create the texture for this source
		_renderwidget->makeCurrent();
		glGenTextures(1, &textureIndex);
		GLclampf lowpriority = 0.1;

		glPrioritizeTextures(1, &textureIndex, &lowpriority);

		// try to create the opencv source
		s = new OpencvSource(opencvIndex, textureIndex, getAvailableDepthFrom(depth));

	} catch (NoCameraIndexException){

		// free the OpenGL texture
		glDeleteTextures(1, &textureIndex);
		// return an invalid pointer
		return 0;
	}

	// scale the source to match the media size
	s->resetScale();

	return ( (Source *) s );
}
#endif

Source *RenderingManager::newAlgorithmSource(int type, int w, int h, double v,
		int p, double depth) {

	// create the texture for this source
	GLuint textureIndex;
	_renderwidget->makeCurrent();
	glGenTextures(1, &textureIndex);
	GLclampf lowpriority = 0.1;
	glPrioritizeTextures(1, &textureIndex, &lowpriority);

	// create a source appropriate for this videofile
	AlgorithmSource *s = new AlgorithmSource(type, textureIndex, getAvailableDepthFrom(depth), w, h, v, p);
	Q_CHECK_PTR(s);

	// scale the source to match the media size
	s->resetScale();

	return ( (Source *) s );
}

Source *RenderingManager::newCloneSource(SourceSet::iterator sit, double depth) {

	// create a source appropriate for this videofile
	CloneSource *s = new CloneSource(sit, getAvailableDepthFrom(depth));
	Q_CHECK_PTR(s);

	// scale the source to match the media size
	s->resetScale();

	return ( (Source *) s );
}

void RenderingManager::insertSource(Source *s){

	if (s) {
		// replace the source name by another available one based on the original name
		s->setName(getAvailableNameFrom(s->getName()));

		// set the last created source to be current
		std::pair<SourceSet::iterator, bool> ret;
		ret = _sources.insert(s);
		if (ret.second)
			setCurrentSource(ret.first);
		else {
			delete s;
	        QMessageBox::warning(0, tr("GLMixer create source"), tr("Could not insert source into the stack."));
		}
	}
}

void RenderingManager::addSourceToBasket(Source *s){

	// add the source into the basket
	dropBasket.insert(s);
	// select no source
	setCurrentSource( getEnd() );
}

int RenderingManager::getSourceBasketSize(){

	return int (dropBasket.size());
}

Source *RenderingManager::getSourceBasketTop(){

	if (dropBasket.empty())
		return 0;
	else
		return (*dropBasket.begin());
}

void RenderingManager::dropSourceWithAlpha(double alphax, double alphay){

	// nothing to drop ?
	if (dropBasket.empty())
		return;
	// get the pointer to the source at the top of the list
	Source *top = *dropBasket.begin();
	// apply the modifications
	top->setAlphaCoordinates(alphax, alphay);
	// insert the source
	insertSource(top);
	// remove from the basket
	dropBasket.erase(top);

}

void RenderingManager::dropSourceWithCoordinates(double x, double y){

	// nothing to drop ?
	if (dropBasket.empty())
		return;

	// get the pointer to the source at the top of the list
	Source *top = *dropBasket.begin();
	// apply the modifications
	top->setX(x);
	top->setY(y);
	// insert the source
	insertSource(top);
	// remove from the basket
	dropBasket.erase(dropBasket.begin());

}

void RenderingManager::dropSourceWithDepth(double depth){

	// nothing to drop ?
	if (dropBasket.empty())
		return;

	// get the pointer to the source at the top of the list
	Source *top = *dropBasket.begin();
	// apply the modifications
	// TODO ; check validity of depth
	top->setDepth(depth);
	// insert the source
	insertSource(top);
	// remove from the basket
	dropBasket.erase(dropBasket.begin());

}

void RenderingManager::removeSource(SourceSet::iterator itsource) {

	// if we are removing the current source, ensure it is not the current one anymore
	if (itsource == _currentSource) {
		_currentSource = _sources.end();
		emit currentSourceChanged(_currentSource);
	}

	if (itsource != _sources.end()) {
		CloneSource *cs = dynamic_cast<CloneSource *> (*itsource);
		if (cs == NULL)
			// first remove every clone of the source to be removed (if it is not a clone already)
			for (SourceList::iterator clone = (*itsource)->getClones()->begin(); clone
					!= (*itsource)->getClones()->end(); clone
					= (*itsource)->getClones()->begin()) {
				removeSource(getById((*clone)->getId()));
			}
		// then remove the source itself
		_sources.erase(itsource);
		delete (*itsource);
	}

	// Disable update of previous frame if all the RenderingSources are deleted
	if (countRenderingSource <= 0) {
		if (previousframe_fbo)
			delete previousframe_fbo;
		previousframe_fbo = 0;
	}
}

void RenderingManager::clearSourceSet() {

	// clear the list of sources
	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its = _sources.begin())
		removeSource(its);

	// reset the id counter
	Source::lastid = 1;
}

bool RenderingManager::notAtEnd(SourceSet::iterator itsource) {
	return (itsource != _sources.end());
}

bool RenderingManager::isValid(SourceSet::iterator itsource) {

	if (notAtEnd(itsource))
		return (_sources.find(*itsource) != _sources.end());
	else
		return false;
}

bool RenderingManager::setCurrentSource(SourceSet::iterator si) {

	if (si != _currentSource) {
		if (notAtEnd(_currentSource))
			(*_currentSource)->activate(false);

		_currentSource = si;
		emit
		currentSourceChanged(_currentSource);

		if (notAtEnd(_currentSource))
			(*_currentSource)->activate(true);

		return true;
	}
	return false;
}

bool RenderingManager::setCurrentSource(GLuint name) {

	return setCurrentSource(getById(name));
}

QString RenderingManager::getAvailableNameFrom(QString name){

	// start with a tentative name and assume it is NOT ok
	QString tentativeName = name;
	bool isok = false;
	int countbad = 2;
	// try to find the name in the list; it is still not ok if it exists
	while (!isok) {
		if ( isValid( getByName(tentativeName) ) ){
			// modify the tentative name and keep trying
			tentativeName = name + QString("-%1").arg(countbad++);
		} else
			isok = true;
	}
	// finally the tentative name is ok
	return tentativeName;
}

double RenderingManager::getAvailableDepthFrom(double depth){

	double tentativeDepth = depth;

	// place it at the front if no depth is provided (default argument = -1)
	if (tentativeDepth < 0)
		tentativeDepth  = (_sources.empty()) ? 0.0 : (*_sources.rbegin())->getDepth() + 1.0;

	tentativeDepth += dropBasket.size();

	// try to find a source at this depth in the list; it is not ok if it exists
	bool isok = false;
	while (!isok) {
		if ( isValid( std::find_if(_sources.begin(), _sources.end(), isCloseTo(tentativeDepth)) ) ){
			tentativeDepth += DEPTH_EPSILON;
		} else
			isok = true;
	}
	// finally the tentative depth is ok
	return tentativeDepth;
}

SourceSet::iterator RenderingManager::changeDepth(SourceSet::iterator itsource,
		double newdepth) {

	if (itsource != _sources.end()) {
		// verify that the depth value is not already taken, or too close to, and adjust in case.
		SourceSet::iterator sb, se;
		double depthinc = 0.0;
		if (newdepth < (*itsource)->getDepth()) {
			sb = _sources.begin();
			se = itsource;
			depthinc = -DEPTH_EPSILON;
		} else {
			sb = itsource;
			sb++;
			se = _sources.end();
			depthinc = DEPTH_EPSILON;
		}
		while (std::find_if(sb, se, isCloseTo(newdepth)) != se) {
			newdepth += depthinc;
		}

		// remember pointer to the source
		Source *tmp = (*itsource);
		// sort again the set by depth: this is done by removing the element and adding it again after changing its depth
		_sources.erase(itsource);
		// change the source internal depth value
		tmp->setDepth(newdepth);

		if (newdepth < 0) {
			// if request to place the source in a negative depth, shift all sources forward
			for (SourceSet::iterator it = _sources.begin(); it
					!= _sources.end(); it++)
				(*it)->setDepth((*it)->getDepth() - newdepth);
		}

		// re-insert the source into the sorted list ; it will be placed according to its new depth
		std::pair<SourceSet::iterator, bool> ret;
		ret = _sources.insert(tmp);
		if (ret.second)
			return (ret.first);
		else
			return (_sources.end());
	}

	return _sources.end();
}

SourceSet::iterator RenderingManager::getById(GLuint id) {

	return std::find_if(_sources.begin(), _sources.end(), hasId(id));
}

SourceSet::iterator RenderingManager::getByName(QString name){

	return std::find_if(_sources.begin(), _sources.end(), hasName(name));
}

/**
 * save and load configuration
 */
QDomElement RenderingManager::getConfiguration(QDomDocument &doc) {

	QDomElement config = doc.createElement("SourceList");

	for (SourceSet::iterator its = _sources.begin(); its != _sources.end(); its++) {

		QDomElement sourceElem = doc.createElement("Source");
		sourceElem.setAttribute("name", (*its)->getName());

		QDomElement pos = doc.createElement("Position");
		pos.setAttribute("X", (*its)->getX());
		pos.setAttribute("Y", (*its)->getY());
		sourceElem.appendChild(pos);

		QDomElement scale = doc.createElement("Scale");
		scale.setAttribute("X", (*its)->getScaleX());
		scale.setAttribute("Y", (*its)->getScaleY());
		sourceElem.appendChild(scale);

		QDomElement d = doc.createElement("Depth");
		d.setAttribute("Z", (*its)->getDepth());
		sourceElem.appendChild(d);

		QDomElement alpha = doc.createElement("Alpha");
		alpha.setAttribute("X", (*its)->getAlphaX());
		alpha.setAttribute("Y", (*its)->getAlphaY());
		sourceElem.appendChild(alpha);

		QDomElement color = doc.createElement("Color");
		color.setAttribute("R", (*its)->getColor().red());
		color.setAttribute("G", (*its)->getColor().green());
		color.setAttribute("B", (*its)->getColor().blue());
		sourceElem.appendChild(color);

		QDomElement blend = doc.createElement("Blending");
		blend.setAttribute("Function", (*its)->getBlendFuncDestination());
		blend.setAttribute("Equation", (*its)->getBlendEquation());
		blend.setAttribute("Mask", (*its)->getMask());
		sourceElem.appendChild(blend);

		QDomElement filter = doc.createElement("Filter");
		filter.setAttribute("Brightness", (*its)->getBrightness());
		filter.setAttribute("Contrast", (*its)->getContrast());
		filter.setAttribute("Pixelated", (*its)->isPixelated());
		filter.setAttribute("ColorTable", (*its)->getColorTable());
		filter.setAttribute("Convolution", (*its)->getConvolution());
		sourceElem.appendChild(filter);

		QDomElement specific = doc.createElement("TypeSpecific");
		specific.setAttribute("type", (*its)->rtti());

		if ((*its)->rtti() == Source::VIDEO_SOURCE) {
			VideoSource *vs = dynamic_cast<VideoSource *> (*its);
			VideoFile *vf = vs->getVideoFile();
			// Necessary information for re-creating this video source:
			// filename, marks, saturation
			QDomElement f = doc.createElement("Filename");
			f.setAttribute("PowerOfTwo", vf->getPowerOfTwoConversion());
			QDomText filename = doc.createTextNode(vf->getFileName());
			f.appendChild(filename);
			specific.appendChild(f);

			QDomElement m = doc.createElement("Marks");
			m.setAttribute("In", vf->getMarkIn());
			m.setAttribute("Out", vf->getMarkOut());
			specific.appendChild(m);

			// TODO  : saturation, if not generic in source, 'isPowerOfTwo'

#ifdef OPEN_CV
		} else if ((*its)->rtti() == Source::CAMERA_SOURCE) {
			OpencvSource *cs = dynamic_cast<OpencvSource *> (*its);

			QDomElement f = doc.createElement("CameraIndex");
			QDomText id = doc.createTextNode(QString::number(cs->getOpencvCameraIndex()));
			f.appendChild(id);
			specific.appendChild(f);
#endif
		} else if ((*its)->rtti() == Source::ALGORITHM_SOURCE) {
			AlgorithmSource *as = dynamic_cast<AlgorithmSource *> (*its);

			QDomElement f = doc.createElement("Algorithm");
			QDomText algo = doc.createTextNode(QString::number(as->getAlgorithmType()));
			f.appendChild(algo);
			specific.appendChild(f);

			// TODO : turbulence width and creation width are not the same
			QDomElement s = doc.createElement("Frame");
			s.setAttribute("Width", as->getFrameWidth());
			s.setAttribute("Height", as->getFrameHeight());
			specific.appendChild(s);

			QDomElement x = doc.createElement("Update");
			x.setAttribute("Periodicity", QString::number(as->getPeriodicity()) );
			x.setAttribute("Variability", as->getVariability());
			specific.appendChild(x);

		} else if ((*its)->rtti() == Source::CLONE_SOURCE) {
			CloneSource *cs = dynamic_cast<CloneSource *> (*its);

			QDomElement f = doc.createElement("CloneOf");
			QDomText name = doc.createTextNode(cs->getOriginalName());
			f.appendChild(name);
			specific.appendChild(f);
		}
		sourceElem.appendChild(specific);

		config.appendChild(sourceElem);
	}

	return config;
}

void applySourceConfig(Source *newsource, QDomElement child) {

	QDomElement tmp;
	newsource->setName( child.attribute("name") );
	newsource->setX( child.firstChildElement("Position").attribute("X").toDouble() );
	newsource->setY( child.firstChildElement("Position").attribute("Y").toDouble() );
	newsource->setScaleX( child.firstChildElement("Scale").attribute("X").toDouble() );
	newsource->setScaleY( child.firstChildElement("Scale").attribute("Y").toDouble() );
	tmp = child.firstChildElement("Alpha");
	newsource->setAlphaCoordinates( tmp.attribute("X").toDouble(), tmp.attribute("Y").toDouble() );
	tmp = child.firstChildElement("Color");
	newsource->setColor( QColor( tmp.attribute("R").toInt(),tmp.attribute("G").toInt(), tmp.attribute("B").toInt() ) );
	tmp = child.firstChildElement("Blending");
	newsource->setBlendEquation( (GLenum) tmp.attribute("Equation").toInt()  );
	newsource->setBlendFunc( GL_SRC_ALPHA, (GLenum) tmp.attribute("Function").toInt() );
	newsource->setMask( (Source::maskType) tmp.attribute("Mask").toInt() );
	tmp = child.firstChildElement("Filter");
	newsource->setBrightness( tmp.attribute("Brightness").toInt() );
	newsource->setContrast( tmp.attribute("Contrast").toInt() );
	newsource->setPixelated( tmp.attribute("Pixelated").toInt() );
	newsource->setColorTable( (Source::colorTableType) tmp.attribute("ColorTable").toInt() );
	newsource->setConvolution( (Source::convolutionType) tmp.attribute("Convolution").toInt() );
}

void RenderingManager::addConfiguration(QDomElement xmlconfig) {

	QList<QDomElement> clones;

	QDomElement child = xmlconfig.firstChildElement("Source");
	while (!child.isNull()) {
		// pointer for new source
		Source *newsource = 0;
		// read the depth where the source should be created
		double depth = child.firstChildElement("Depth").attribute("Z").toDouble();

		// get the type of the source to create
		QDomElement t = child.firstChildElement("TypeSpecific");
		Source::RTTI type = (Source::RTTI) t.attribute("type").toInt();

		// create the source according to its specific type information
		if (type == Source::VIDEO_SOURCE ){
			// read the tags specific for a video source
			QDomElement Filename = t.firstChildElement("Filename");
			QDomElement marks = t.firstChildElement("Marks");

			// create the video file
			VideoFile *newSourceVideoFile = NULL;
			if ( !Filename.attribute("PowerOfTwo").toInt() && (glSupportsExtension("GL_EXT_texture_non_power_of_two") || glSupportsExtension("GL_ARB_texture_non_power_of_two") ) )
				newSourceVideoFile = new VideoFile(this);
			else
				newSourceVideoFile = new VideoFile(this, true, SWS_FAST_BILINEAR, PIX_FMT_RGB32);
			// if the video file was created successfully
			if (newSourceVideoFile){
				// TODO : forward error messages to display
				// QObject::connect(newSourceVideoFile, SIGNAL(error(QString)), this, SLOT(displayErrorMessage(QString)));
				// can we open the file ?
				if ( newSourceVideoFile->open( Filename.text(), marks.attribute("In").toLong(), marks.attribute("Out").toLong() ) ) {
					// create the source as it is a valid video file (this also set it to be the current source)
					newsource = RenderingManager::getInstance()->newMediaSource(newSourceVideoFile, depth);
					if (!newsource)
				        QMessageBox::warning(0, tr("GLMixer create source"), tr("Could not create media source %1. ").arg(child.attribute("name")));
				}
			}

#ifdef OPEN_CV
		} else if ( type == Source::CAMERA_SOURCE ) {
			QDomElement camera = t.firstChildElement("CameraIndex");

			newsource = RenderingManager::getInstance()->newOpencvSource( camera.text().toInt(), depth);
			if (!newsource)
		        QMessageBox::warning(0, tr("GLMixer create source"), tr("Could not create camera source %1 with devide index %2. ").arg(child.attribute("name")).arg(camera.text()));
#endif

		} else if ( type == Source::ALGORITHM_SOURCE) {
			// read the tags specific for an algorithm source
			QDomElement Algorithm = t.firstChildElement("Algorithm");
			QDomElement Frame = t.firstChildElement("Frame");
			QDomElement Update = t.firstChildElement("Update");

			newsource = RenderingManager::getInstance()->newAlgorithmSource(Algorithm.text().toInt(),
					Frame.attribute("Width").toInt(), Frame.attribute("Height").toInt(),
					Update.attribute("Variability").toDouble(), Update.attribute("Periodicity").toInt(), depth);
			if (!newsource)
		        QMessageBox::warning(0, tr("GLMixer create source"), tr("Could not create algorithem source %1. ").arg(child.attribute("name")));


		} else if ( type == Source::RENDERING_SOURCE) {
			// no tags specific for a rendering source
			newsource = RenderingManager::getInstance()->newRenderingSource(depth);
			if (!newsource)
		        QMessageBox::warning(0, tr("GLMixer create source"), tr("Could not create rendering loopback source %1. ").arg(child.attribute("name")));

		} else if ( type == Source::CLONE_SOURCE) {
			// remember the node of the sources to clone
			clones.push_back(child);
		}


		if (newsource) {
			// Apply parameters to the created source
			applySourceConfig(newsource, child);
			// insert the source in the scene
			insertSource(newsource);
		}
		child = child.nextSiblingElement();
	}

	// Process the list of clones names ; now that every source exist, we can be sure they can be cloned
    QListIterator<QDomElement> it(clones);
    while (it.hasNext()) {

		Source *clonesource = 0;

    	QDomElement c = it.next();
    	double depth = c.firstChildElement("Depth").attribute("Z").toDouble();
    	QDomElement t = c.firstChildElement("TypeSpecific");
		QDomElement f = t.firstChildElement("CloneOf");
		// find the source which name is f.text()
    	SourceSet::iterator cloneof =  getByName(f.text());
    	if (isValid(cloneof)) {
    		clonesource = RenderingManager::getInstance()->newCloneSource(cloneof, depth);
    		if (clonesource) {
    			// Apply parameters to the created source
    			applySourceConfig(clonesource, c);
    			// insert the source in the scene
    			insertSource(clonesource);
    		}else
    	        QMessageBox::warning(0, tr("GLMixer create source"), tr("Could not create clone source %1.").arg(c.attribute("name")));
    	} else {
    		QMessageBox::warning(0, tr("GLMixer create source"), tr("The source '%1' cannot be the clone of '%2' ; no such source.").arg(c.attribute("name")).arg(f.text()));
    	}
    }


}

