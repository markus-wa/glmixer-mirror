/*
 * MixingToolboxWidget.h
 *
 *  Created on: Sep 2, 2012
 *      Author: bh
 */

#ifndef MIXINGTOOLBOXWIDGET_H_
#define MIXINGTOOLBOXWIDGET_H_

#include <qwidget.h>
#include "ui_MixingToolboxWidget.h"


#include "Source.h"

class MixingToolboxWidget: public QWidget, public Ui::MixingToolboxWidget {

    Q_OBJECT

public:

    MixingToolboxWidget(QWidget *parent);
    void setAntialiasing(bool antialiased);

public Q_SLOTS:

    void connectSource(SourceSet::iterator);
    void updateSource();

private:

	class GammaLevelsWidget *gammaAdjust;
	class SourceDisplayWidget *sourcepreview;
    Source *source;
};

#endif /* MIXINGTOOLBOXWIDGET_H_ */
