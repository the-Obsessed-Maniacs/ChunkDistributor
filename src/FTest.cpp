#include "FTest.h"

#include <QChart>
#include <QLineSeries>
#include <QLogValueAxis>
#include <QPainterPath>
#include <QStatusBar>
#include <QValueAxis>
#include <QWheelEvent>

FTest::FTest( QWidget *parent )
	: QMainWindow( parent )
{
	setupUi( this );
	v->viewport()->installEventFilter( this );

	// Die Darstellung der Notenfrequenzen ... basteln wir uns mal ein Diagramm:
	// 0..12kHz => F-Achse
	// 0..65536 => Y-Achse
	// alle 96 Noten einzeichnen
	qDebug() << "| Note |  freq [Hz]  | SIDfr | COf |";
	for ( quint16 lv = 0, n = 0; lv != 65535; n++ )
	{
		// (oct)4*12 = 48; +9 [C→A] = 57
		NoteFreqs.append( 432.0 * pow( 2, ( ( n - 9 ) / 12. ) - 4. ) );
		auto fs = NoteFreqs.last() * pow( 2, 24 ) / 985248;
		if ( fs < 65535. ) lv = qRound( fs );
		else lv = 65535;
		SIDnotes.append( lv );
		// C-0 kann der Filter nicht... abfangen als "silent"!
		auto co = qRound( ( NoteFreqs.last() - 20 ) / 6 );
		SID_COs.append( co < 1 ? 1u : co );
		qDebug().noquote() << tr( "|  %1  | %2 | $%3 |$%4 |" )
								  .arg( n, 2 )
								  .arg( NoteFreqs[ n ], 11 )
								  .arg( SIDnotes[ n ], 4, 16, QChar{ '0' } )
								  .arg( SID_COs[ n ], 3, 16, QChar{ '0' } );
	}
	redisplay();
}

FTest::~FTest() {}

void FTest::redisplay()
{
	auto c = new QChart;
	c->setPreferredSize( 1280, 720 );
	c->setAnimationOptions( QChart::AnimationOption::NoAnimation );
	c->setBackgroundRoundness( 5. );
	QXYSeries *nfsid = new QLineSeries( this );
	nfsid->setName( SID_noteoffs != 0 ? tr( "Note(%1) → SIDfreq" ).arg( SID_noteoffs )
									  : tr( "Note → SIDfreq" ) );
	QXYSeries *nfco = new QLineSeries( this );
	nfco->setName( co5->isChecked() ? "Note → FLTco[11:5]" : "Note → FLTco" );
	auto f_n = nn->isChecked();
	for ( int n = 0; n < NoteFreqs.count(); n++ )
	{
		nfsid->append( f_n ? ( n + SID_noteoffs ) % NoteFreqs.count()
						   : NoteFreqs[ ( n + SID_noteoffs ) % NoteFreqs.count() ],
					   SIDnotes[ n ] );
		nfco->append( f_n ? n : NoteFreqs[ n ], SID_COs[ n ] << ( co5->isChecked() ? 5 : 0 ) );
	}
	QAbstractAxis *vx, *vy;
	if ( l10x->isChecked() && !f_n )
	{
		auto lvx = new QLogValueAxis;
		lvx->setBase( 10 );
		vx = lvx;
	} else vx = new QValueAxis;
	if ( ldy->isChecked() )
	{
		auto lvy = new QLogValueAxis;
		lvy->setBase( 2 );
		vy = lvy;
	} else vy = new QValueAxis;

	vx->setRange( f_n ? xToValues->isChecked() ? NoteFreqs.first() : 1.,
				  xToValues->isChecked() ? NoteFreqs.last() : 12500.
					  : 0.,
				  96. );
	vy->setRange( 1., 65536. );

	c->addAxis( vx, Qt::AlignBottom );
	c->addAxis( vy, Qt::AlignLeft );
	c->addSeries( nfsid );
	c->addSeries( nfco );

	nfsid->attachAxis( vx );
	nfsid->attachAxis( vy );
	nfco->attachAxis( vx );
	nfco->attachAxis( vy );

	auto s = new QGraphicsScene( this );
	s->addItem( c );
	v->setScene( s );
	Ui::FTest::statusBar->showMessage( tr( "redisplayed with offset: %1" ).arg( SID_noteoffs ),
									   1000 );
}

void FTest::addOffs( int offs )
{
	SID_noteoffs += offs;
	redisplay();
}

bool FTest::eventFilter( QObject *fo, QEvent *ev )
{
	if ( ( fo == v->viewport() ) && ( ev->type() == QEvent::Wheel ) )
	{
		auto e = reinterpret_cast< QWheelEvent * >( ev );
		if ( e->modifiers().testFlag( Qt::KeyboardModifier::ControlModifier ) )
		{
			// Handle zoom Event => value
			auto z = 1. + ( e->angleDelta().y() / QWheelEvent::DefaultDeltasPerStep ) * 0.05;
			if ( e->modifiers().testAnyFlags( Qt::ShiftModifier | Qt::AltModifier ) )
				v->scale( e->modifiers().testFlag( Qt::KeyboardModifier::AltModifier ) ? z : 1.,
						  e->modifiers().testFlag( Qt::KeyboardModifier::ShiftModifier ) ? z : 1. );
			else v->scale( z, z );
			return true;
		}
	}
	return false;
}


int main( int argc, char *argv[] )
{
	QApplication app( argc, argv );
	app.setStyle( "Fusion" );
	FTest window;
	window.show();
	return app.exec();
}
