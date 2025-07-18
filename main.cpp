#include "PackerTester.h"

#include <QtWidgets/QApplication>

int main( int argc, char *argv[] )
{
	QApplication app( argc, argv );
	app.setStyle( "Fusion" );
	PackerTester window;
	window.show();
	return app.exec();
}
