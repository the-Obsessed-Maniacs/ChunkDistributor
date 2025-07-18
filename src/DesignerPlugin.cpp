#include "DesignerPlugin.h"

using namespace Qt::StringLiterals;

QIcon DesignerPlugin::icon() const
{
	return QIcon();
}

QString DesignerPlugin::domXml() const
{
	return uR"(<ui language="c++" displayname="Algo">
    <widget class="Algo::Algo" name="algo">
    </widget>
</ui>)"_s;
}

QString DesignerPlugin::group() const
{
	return u"Wurstbrote"_s;
}

QString DesignerPlugin::includeFile() const
{
	return u"Algo.h"_s;
}
QString DesignerPlugin::name() const
{
	return u"Algo"_s;
}

QString DesignerPlugin::toolTip() const
{
	return u"blah for blubb."_s;
}

QString DesignerPlugin::whatsThis() const
{
	return u"Algo ist ein Wurstbrot zum Beschmieren - oder nicht?"_s;
}


void DesignerPlugin::initialize( QDesignerFormEditorInterface * core )
{
	if ( initialized ) return;

	initialized = true;
}
