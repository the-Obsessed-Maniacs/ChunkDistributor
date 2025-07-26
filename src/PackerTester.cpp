#include "PackerTester.h"

#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QResource>

using namespace Qt::StringLiterals;
// Texte:
/////////
#define MSX_ID 3

const char *msx[][ 6 ] = {
	{ "Insanity", "186e",
	  "6,6,135,135,16,16,74,74,24,24,24,24,24,24,24,24,24,24,24,18,18,20,22,59,24,26,24,28,26,28,"
	  "25,211,49,22,13,15,36,18,52,26,76,49,18,81,8,8,55,53,53,79,76,35,94,3,27,59,17,16,74,106,"
	  "105,94,35,90,46,98,47,40,20,23,69,68,67,62,92,68,38,29,36,68,38,59,25,76,11,65,125,68,68,"
	  "105,90,110,8,1",
	  "2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,87,88",
	  "19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,"
	  "49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,"
	  "79,80,81,82,83,84,85,86,89,90,91",
	  "0,1,92,93" },
	{ "Flyland", "1864",
	  "142,142,37,37,29,29,21,21,21,21,21,21,21,21,21,21,21,19,19,23,16,19,23,35,23,13,19,13,40,35,"
	  "137,92,18,58,44,20,29,106,19,29,33,58,106,20,47,6,49,34,34,4,35,41,19,106,81,52,16,35,50,10,"
	  "13,26,10,34,36,38,45,42,45,140,140,140,140,13,10,12,13,60,60,73,68,49,11,30,30,16,2",
	  "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,77,78",
	  "17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,"
	  "47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,"
	  "79,80,81,82,83,84",
	  "85,86" },
	{ "Confusion", "1864",
	  "7,7,93,93,14,14,36,36,11,11,11,11,11,11,11,11,11,11,11,22,18,1,8,13,11,18,6,7,19,68,32,28,"
	  "21,24,8,4,31,18,19,47,20,25,23,21,70,6,29,25,15,22,56,45,22,25,99,69,6,10,22,4,41,41,117,"
	  "146,125,100,112,107,43,45,39,24,3",
	  "2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,60,61",
	  "19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,"
	  "49,50,51,52,53,54,55,56,57,58,59,62,63,64,65,66,67,68,69,70",
	  "0,1,71,72" },
	{ "Da Groove", "186f",
	  "1,1,81,81,26,26,69,69,13,13,13,13,13,13,13,13,13,13,13,58,39,34,3,47,18,24,35,53,15,41,35,"
	  "35,43,59,43,66,15,35,21,35,57,3,58,54,56,54,42,41,35,35,31,31,29,20,45,3,3,3,3,1,1,24,3",
	  "2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,50,51",
	  "19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,"
	  "49,52,53,54,55,56,57,58,59,60",
	  "0,1,61,62" },
	{ "Happyness", "1864",
	  "82,82,26,26,22,22,9,9,9,9,9,9,9,9,9,9,9,29,29,29,3,14,71,107,11,13,18,26,25,26,29,47,64,28,"
	  "33,67,143,58,60,165,14,99,37,91,53,39,6,56,60,18,36,19,75,75,58,38,44,69,37,47,47,53,6,49,"
	  "10,48,48,29,37,52,9,10,9,4,7,8,24,3",
	  "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,65,66",
	  "17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,"
	  "47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,67,68,69,70,71,72,73,74,75",
	  "76,77" } };

PackerTester::PackerTester( QWidget *parent )
	: QMainWindow( parent )
{
	auto lc = locale(); // make a copy
	auto no = lc.numberOptions();
	no.setFlag( QLocale::OmitGroupSeparator, false );
	lc.setNumberOptions( no );
	setLocale( lc );
	// private connections:
	connect( this, &PackerTester::mache, this, &PackerTester::machen, Qt::QueuedConnection );
	// UI
	setupUi( this );
	QResource r( ":/ReadMe.md" );
	if ( r.isValid() ) t_algo->setDesc( r.uncompressedData() );
	bg->setId( rb_boosted, 0 );
	bg->setId( rb_visual, 1 );
	bg->setId( rb_animated, 2 );
	v->setScene( new QGraphicsScene( this ) );
	v->viewport()->installEventFilter( this );
	v->setMouseTracking( true );
	v->setAlignment( Qt::AlignHCenter | Qt::AlignBottom );
	for ( int i = 0; i < ( sizeof( msx ) / sizeof( msx[ 0 ] ) ); ++i )
		cb_tune->addItem( msx[ i ][ 0 ] );
	cb_tune->setCurrentIndex( MSX_ID );
	auto a = new QVariantAnimation( this ); // sl_spd, "value"
	connect( a,
			 &QVariantAnimation::valueChanged,
			 [ & ]( const QVariant &v )
			 {
				 setWindowOpacity( v.toDouble() );
				 sl_spd->setValue( 1000 - v.toDouble() * 750 );
			 } );
	a->setKeyValues( QVariantAnimation::KeyValues{ { 0.0, 0.0 }, { 1.0, 1.0 } } );
	a->setDuration( 1500 );
	a->setEasingCurve( QEasingCurve::OutBounce );
	a->start( QAbstractAnimation::DeleteWhenStopped );

	connect( t_algo,
			 &Algo::Algorithm::stateMessageChanged,
			 [ this ]( QString msg ) { sb->showMessage( msg ); } );
}

PackerTester::~PackerTester() {}

bool PackerTester::eventFilter( QObject *o, QEvent *e )
{
	if ( o == v->viewport() ) switch ( e->type() )
		{
			case QEvent::Wheel:
				{
					auto we = ( QWheelEvent * ) e;
					// To not disturb the internal Scroll Area, only accept CTRL + Wheel
					if ( we->modifiers() & Qt::CTRL )
					{
						// Qt Docs state angleDelta is in 1/8th of Degrees, anyhow, they also
						// provide "QWheelEvent::DefaultDeltasPerStep" - much more useful!
						// -> scaled into [-1..1] (i.e. 1.0 is a full rotation)
						qreal d = we->angleDelta().y() / QWheelEvent::DefaultDeltasPerStep;
						// optionally check a modifier to do sharper scaling
						if ( we->modifiers() & Qt::SHIFT ) d *= 0.1;
						else d *= 0.05;
						// also protect against flipping due to negative scale
						v->scale( qMax( 1. + d, 0.1 ), qMax( 1. + d, 0.1 ) );
						e->accept();
						return zoomed = true;
					}
				}
				break;
			case QEvent::MouseButtonPress:
				if ( ( ( QMouseEvent * ) e )->button() == Qt::MiddleButton )
					return !( zoomed = false );
				break;
		}
	return false;
}

void PackerTester::on_cb_tune_currentIndexChanged( int i )
{
	if ( i >= 0 && i < ( sizeof( msx ) / sizeof( msx[ 0 ] ) ) )
	{
		te_sizes->setText( msx[ i ][ 2 ] );
		le_hPrio->setText( msx[ i ][ 3 ] );
		le_nPrio->setText( msx[ i ][ 4 ] );
		le_lPrio->setText( msx[ i ][ 5 ] );
		sb_start->setValue( QString( msx[ i ][ 1 ] ).toInt( nullptr, 16 ) );
		if ( state == 0 )
		{
			button->setEnabled( true );
			button->setText( "Start!" );
		}
	} else {
		te_sizes->clear();
		le_hPrio->clear();
		le_nPrio->clear();
		le_lPrio->clear();
		sb_start->setValue( QString( "186f" ).toInt( nullptr, 16 ) );
	}
	button->setIcon( QIcon::fromTheme( ( state & pause ) || state == 0
										   ? QIcon::ThemeIcon::MediaPlaybackStart
										   : QIcon::ThemeIcon::MediaPlaybackPause ) );
	clear_data();
}

void PackerTester::on_button_clicked()
{
	// Neu:  Idle und keine Chunk-Größen -> nix tun
	if ( state == 0 && ( te_sizes->toPlainText().isEmpty() || chunk_mgr != nullptr ) ) return;
	if ( state == 0 ) // Berechnung starten ...
	{
		s_id = -1;
		naechste_seite();
		for ( int i = 0; i < tabs->count(); i++ )
			tabs->setTabEnabled( i, i == tabs->currentIndex() );
	} else if ( state & pause ) {
		state ^= pause;
		emit mache( weiter_mit );
	}
	if ( ( state & pause ) == 0 ) button->setText( "Pause" );
	button->setIcon( QIcon::fromTheme( ( state & pause ) || state == 0
										   ? QIcon::ThemeIcon::MediaPlaybackStart
										   : QIcon::ThemeIcon::MediaPlaybackPause ) );
}

void PackerTester::on_sl_spd_valueChanged( int h )
{
	PositionAnimation::animationDuration = h + 1;
}

void PackerTester::on_bg_idPressed( int button_id )
{
	// Einer der 3 Visualisierungsoption-Buttons wird gerade gedrückt.  Erst Mal Pause
	// einschalten, damit keine Verschlucker entstehen.  Und die aktuelle Lösung ins Visier
	// animieren.  Soviel Zeit muss beim Umschalten sein ;)
	if ( state != 0 ) state |= pause_requested;
	if ( boosted && button_id != 0 )
	{
		boosted = false;
		rebuild_results();
	}
	switch ( button_id )
	{
		case 0: boosted = !( animated = false ); break;
		case 1: boosted = animated = false; break;
		case 2: boosted = !( animated = true ); break;
	}
}

void PackerTester::on_bg_idReleased( int button_id )
{
	// Der gedrückte Visualisierungsoption-Button wurde wieder los gelassen - Pause
	// abschliessen, je nach Zustand Bild zurecht rücken.
	aktualisiere_anzeige();
	if ( state & pause ) emit mache( weiter_mit );
	state &= ~( pause | pause_requested );
}

void PackerTester::rebuild_results()
{ // Muss die bisherigen Ergebnisse noch zusammenbauen ... ich weiss leider nicht, ab wann
	// der boosted-Modus eingesetzt hat, also setze ich die Stages zurück, wo die Listen nicht
	// übereinstimmen, um danach genau diese Seiten wieder neu zusammenzubauen.
	for ( int s_id = 0; s_id < seiten.count(); ++s_id )
		if ( seiten[ s_id ]->solution() != ergebnisse[ s_id ].second )
			seiten[ s_id ]->prepareReplace( ergebnisse[ s_id ].second, true );

	for ( int s_id = 0; s_id < seiten.count(); ++s_id )
		if ( seiten[ s_id ]->solution() != ergebnisse[ s_id ].second )
			seiten[ s_id ]->replaceSolution( ergebnisse[ s_id ].second, true );
	aktualisiere_anzeige();
}


void PackerTester::nimm_vorrat()
{
	auto id = vorrat.takeAt( v_id );
	ergebnisse[ s_id ].second.append( id );
	rest -= daten[ id ]->bytes();
	if ( !boosted ) seiten[ s_id ]->nimm( id, animated );
}

void PackerTester::finde_position( ChunkObj *co )
{
	// Hier noch einmal die EinSortierung in die Liste ... mir ist soeben bewusst geworden, dass
	// diese Einsortierung sowohl nach Größe, aber innerhalb gleicher Größen auch nach Priorität
	// und ID vollzogen werden MUSS!  Sonst bleibt der Algorithmus "hängen".
	v_id = 0;
	while ( v_id < vorrat.size() )
		if ( const auto cmp = daten[ vorrat[ v_id ] ] ) // sollte immer true werden!
			if ( co->bytes() < cmp->bytes() ) ++v_id;
			else if ( ( co->bytes() == cmp->bytes() )
					  && ( co->prio() < cmp->prio() || co->id() > cmp->id() ) )
				++v_id;
			else break;
}

void PackerTester::pack_vorrat()
{
	if ( ergebnisse[ s_id ].second.isEmpty() ) return; // nix mehr weg zu packen
	auto id = ergebnisse[ s_id ].second.takeLast();
	rest += daten[ id ]->bytes();
	finde_position( daten[ id ] );
	vorrat.insert( v_id++, id ); // v_id am Ende inkrementieren!
	if ( !boosted ) seiten[ s_id ]->pack( id, animated );
}

void PackerTester::aktualisiere_anzeige()
{
	if ( boosted && ( state & ~pause ) > 0 )
	{
		if ( lbn != ( s_id | ( quint64( state ) << 16 ) ) )
			sb->showMessage( u"Seitenaktualisierung: aktiv = $%1, State: %2, Iteration: %L3"_s
								 .arg( seiten[ s_id ]->addr(), 4, 16, QChar{ '0' } )
								 .arg( state )
								 .arg( it_cnt ) ),
				lbn = s_id | ( quint64( state ) << 16 );
	} else if ( chunk_mgr ) {
		chunk_mgr->adjustSize();
		auto  lr = chunk_mgr->geometry();
		// alles wird Parallel animiert
		auto  vs = style()->pixelMetric( QStyle::PM_LayoutVerticalSpacing );
		qreal dy = 3 * ( vs + QFontMetricsF( font() ).height() );
		qreal sy = -dy;
		int	  i;
		// Die aktuell bearbeitete Seite ist über dem Vorrat, alle vorigen Seiten darüber.
		auto  aniSeite = [ & ]( qreal delta )
		{
			if ( !qFuzzyCompare( seiten[ i ]->property( PositionAnimation::POS ).toPointF().y(),
								 sy ) )
				seiten[ i ]->animateTo( { 0., sy }, { -256., 0. }, { 256., 0. } );
			sy += delta;
		};
		for ( i = s_id; i >= 0; i-- ) aniSeite( -dy );
		lr.setTop( sy + dy );
		// Und zurückgestellte Seiten sind unterhalb des Vorrates ...
		sy = lr.bottom() + style()->pixelMetric( QStyle::PM_LayoutVerticalSpacing );
		for ( i = s_id + 1; i < seiten.size(); ++i ) aniSeite( dy );
		if ( sy > lr.bottom() + dy ) lr.setBottom( sy );
		else lr.setBottom( lr.bottom() + dy );
		lr.setRight( v->scene()->sceneRect().right() );
		// Am Ende der Animation sollte der View zurecht gestuppst werden ...
		if ( !zoomed ) v->fitInView( lr.adjusted( -vs, -vs, 0, vs ), Qt::KeepAspectRatio );
	}
}

void PackerTester::tabs_einschalten()
{
	for ( int i = 0; i < tabs->count(); i++ ) tabs->setTabEnabled( i, true );
}


void PackerTester::clear_data()
{
	v->scene()->clear(); // Dieser Call macht die internen Pointer ungültig -> überschreiben!
	chunk_mgr = nullptr;
	daten.clear();
	vorrat.clear();
	seiten.clear();
	ergebnisse.clear();
}

void PackerTester::naechste_seite()
{
	// qDebug() << "PackerTester::naechste_seite()";
	if ( ++s_id == 0 ) // Gesamtstart erbeten
	{
		// Ok, wo fange ich an? Ich habe da die Eingabe, daraus baue ich jetzt Mengen.
		adr0 = sb_start->value();
		rest = ( ( adr0 + 256 ) & 0xff00 ) - adr0;
		v->scene()->addItem( chunk_mgr = new Chunks( nullptr ) );
		v->scene()->addItem( seiten.emplace_back( new Stage( adr0, chunk_mgr ) ) );
		// pack_vorrat entnimmt die ID aus dem Ergebnis - die Erstellung der Elemente
		// spielt einfach ein Ergebnis vor, um das Element einzusortieren.
		ergebnisse.emplace_back( adr0, QList< int >() );
		// Eingabefelder auslesen und interpretieren
		QList< uint > ph, pn, pl;
		for ( auto t : le_hPrio->text().split( ',' ) ) ph << t.toUInt();
		for ( auto t : le_nPrio->text().split( ',' ) ) pn << t.toUInt();
		for ( auto t : le_lPrio->text().split( ',' ) ) pl << t.toUInt();
		for ( auto sztx : te_sizes->toPlainText().split( ',' ) )
		{
			auto id = daten.size();
			auto sz = sztx.toUInt();
			// finde prio
			uint p	= ( ph.indexOf( id ) >= 0 ) ? 2 : ( pn.indexOf( id ) >= 0 ) ? 1 : 0;
			auto co = new ChunkObj( chunk_mgr, Chunk{ sz, p, static_cast< uint >( id ) } );
			daten.append( co );
			// Optisch alle Chunks ins Rennen werfen ;)
			chunk_mgr->addChunk( co );
			// Verfügbarmachen des Datenelementes
			finde_position( co );
			vorrat.insert( v_id, id );
		}
		// Abschliessende Vorbereitungen:
		state = 1, v_id = it_cnt = 0;
	} else {
		// Nächste Seite starten - hierbei ist mir aufgefallen: das chunk-placement-plugin
		// sollte die Versätze der verfügbaren Seiten kennen.  Beispielsweise, wenn ein
		// Jitter-NMI mit der Ninja-Methode verwendet wurde, könnte man diesen wunderbar mit den
		// Musik-Daten "auffüllen".
		++state, v_id = 0, rest = 256;
		if ( seiten.size() > s_id )
		{ // Seite existiert schon - wurde schon mal back-ge-tracked ...
			adr0 = seiten[ s_id ]->addr();
			// Rest anhand des bestehenden Ergebnisses neu berechnen
			for ( auto cid : ergebnisse[ s_id ].second ) rest -= daten[ cid ]->bytes();
		} else {
			adr0 = ( ( adr0 + 256 ) & 0xff00 );
			seiten.append( new Stage( adr0, chunk_mgr ) );
			v->scene()->addItem( seiten.last() );
			ergebnisse.emplace_back( adr0, QList< int >() );
		}
	}
	aktualisiere_anzeige();
	emit warte( iterate );
}

void PackerTester::finde_chunks()
{
	if ( state & pause_requested ) return do_pause( iterate );
	++it_cnt;
	while ( v_id < vorrat.size() && rest > 0 )
	{
		if ( rest >= daten[ vorrat[ v_id ] ]->bytes() ) nimm_vorrat();
		else ++v_id;
	}
	warte( vorrat.empty() ? solved : rest == 0 ? page_solved : backtrack );
}

void PackerTester::zurueck()
{
	if ( state & pause_requested ) return do_pause( backtrack );
	// if ( s_id == 15 )	// Hit#1:291436307,8,9,10 (ganz normales Arbeiten - es sind einfach so
	// so so
	//	__debugbreak(); // so so viele Möglichkeiten...

	while ( v_id >= vorrat.size() && !ergebnisse[ s_id ].second.isEmpty() ) pack_vorrat();

	if ( v_id < vorrat.size() ) aktualisiere_anzeige(), warte( iterate );
	else if ( s_id == 0 && ergebnisse.first().second.isEmpty() ) warte( not_solved );
	else
	{ // Backtracking der Seite vorher einleiten ...
		--s_id, --state;
		adr0 = seiten[ s_id ]->addr();
		rest = ( ( adr0 + 256 ) & 0xff00 ) - adr0;
		for ( auto id : ergebnisse[ s_id ].second ) rest -= daten[ id ]->bytes();
		aktualisiere_anzeige();
		warte( backtrack );
	}
}

void PackerTester::keine_loesung()
{
	abschluss( tr( "SCHADE!" ),
			   tr( "Auch nach %1 Iterationen keine Lösung gefunden!" ).arg( it_cnt ) );
}

void PackerTester::loesung_gefunden()
{
	abschluss( tr( "FERTIG!" ),
			   tr( "Berechnungen abgeschlossen. Nur %1 Iterationen ;) =*-" ).arg( it_cnt ) );
}

void PackerTester::abschluss( QString ButtonText, QString MSG_Tx )
{
	weiter_mit = nothing;
	state	   = 0;
	button->setText( ButtonText );
	sb->showMessage( MSG_Tx );
	if ( boosted ) rebuild_results();
	aktualisiere_anzeige();
	tabs_einschalten();
	QSignalBlocker b( cb_tune );
	cb_tune->setCurrentIndex( -1 );
}

void PackerTester::timerEvent( QTimerEvent *event )
{
	if ( PositionAnimation::runningAnimations == 0 )
	{
		killTimer( event->timerId() );
		emit mache( weiter_mit );
	}
}

void PackerTester::warte( continuation cont )
{
	// Finde heraus, ob eine Animation läuft:
	if ( !boosted && PositionAnimation::runningAnimations > 0 )
	{
		weiter_mit = cont;
		startTimer( 10 );
	} else emit mache( cont );
}


void PackerTester::machen( continuation cont )
{
	switch ( cont )
	{
		case nothing: break;
		case iterate: return finde_chunks();
		case backtrack: return zurueck();
		case page_solved: return naechste_seite();
		case solved: return loesung_gefunden();
		case not_solved: return keine_loesung();
	}
}

void PackerTester::do_pause( continuation cont )
{
	state	   = ( state & ~pause_requested ) | pause;
	weiter_mit = cont;
	if ( cont != continuation::nothing )
		button->setText( u"Weiter"_s ),
			button->setIcon( QIcon::fromTheme( QIcon::ThemeIcon::MediaPlaybackStart ) );
}
