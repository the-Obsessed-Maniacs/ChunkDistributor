#pragma once

#define _DESIGNER_
#include "Algo.h"

#include <QDesignerCustomWidgetInterface>
#include <QObject>

class DesignerPlugin
	: public QObject
	, public QDesignerCustomWidgetInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA( IID "org.qt-project.Qt.QDesignerCustomWidgetInterface" )
	Q_INTERFACES( QDesignerCustomWidgetInterface )

  public:
	explicit DesignerPlugin( QObject *parent = nullptr )
		: QObject( parent )
	{}
	bool	 isContainer() const override { return false; }
	bool	 isInitialized() const override { return initialized; }
	QIcon	 icon() const override;
	QString	 domXml() const override;
	QString	 group() const override;
	QString	 includeFile() const override;
	QString	 name() const override;
	QString	 toolTip() const override;
	QString	 whatsThis() const override;
	QWidget *createWidget( QWidget *parent ) override { return new Algo::Algo( parent ); }
	void	 initialize( QDesignerFormEditorInterface *core ) override;

  private:
	bool initialized = false;
};
