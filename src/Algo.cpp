
#include "Algo.h"

#include "AlgoGfx.h"

#include <QChronoTimer>
#include <QRandomGenerator>
#include <QThread>

namespace Algo
{
	Algo::Algo( QWidget *parent )
		: QWidget( parent )
		, idleProc( new QChronoTimer( this ) )
	{
		setupUi( this );
		// Initial State: description is hidden in any case
		on_tb_hideExpl_clicked();
		// Prio-Display korrekt initialisieren
		setNPrio( sb_prio_cnt->value() );
		// Visual Correction Button erstmal verstecken
		pb_viscor->hide();
		// Die Erklärungsanzeige ist programmiert ;)
		// Weiter geht es ... die Zufallsgeneration implementieren
		for ( auto [ i, a ] : { QPair{ sb_minSz, sb_maxSz },
								{ sb_minCcnt, sb_maxCcnt },
								{ sb_minBoffs, sb_maxBoffs },
								{ sb_minBcnt, sb_maxBcnt } } )
			connect( i, &QSpinBox::valueChanged, a, &QSpinBox::setMinimum ),
				connect( a, &QSpinBox::valueChanged, i, &QSpinBox::setMaximum );
		connect( sb_prio_cnt, &QSpinBox::valueChanged, this, &Algo::setNPrio );
		// Animation-Display: die Button-Group braucht noch Informationen, die im Designer nicht
		// gesetzt werden können.
		bg_visMode->setId( rb_silent, silent );
		bg_visMode->setId( rb_visual, visual );
		bg_visMode->setId( rb_animated, animated );
		// Und das Viewport-Wheel-Zooming ...
		v->viewport()->installEventFilter( this );
		v->setMouseTracking( true );
		// Idle-Processing: wenn der Rechenthread zu schnell ist und die MessageQueue befüllt, kann
		// nicht Animiert werden.  Das heisst: die gemeldeten Rechenergebnisse werden lediglich
		// zwischengespeichert, ein Flag gesetzt und soll später bei "idle" durchgeführt werden.
		connect( idleProc, &QChronoTimer::timeout, this, &Algo::idle_processing );
	}

	Algo::~Algo()
	{
		if ( runnerThread ) runnerThread->quit(), runnerThread->wait();
	}

	bool Algo::eventFilter( QObject *o, QEvent *e )
	{
		// QGraphicsView Mouse Wheel Zoom - CTRL + Wheel on the view to scale
		if ( o == v->viewport() && e->type() == QEvent::Wheel )
		{
			auto we = reinterpret_cast< QWheelEvent * >( e );
			// only use events with CTRL pressed!
			if ( we->modifiers() & Qt::CTRL )
			{
				auto d = 1. // double the scaling factor if SHIFT is pressed, also
						 + ( we->angleDelta().y() ) / QWheelEvent::DefaultDeltasPerStep
							   * ( we->modifiers() & Qt::SHIFT ? 0.1 : 0.05 );
				d = qMin( 1.1, qMax( 0.9, d ) ); // restrict too much scaling
				v->scale( d, d );
				e->accept();
				return true;
			}
		}
		return false;
	}

	// Stuff needed for the Designer implementation
	void Algo::setDesc( QString md )
	{
		te_expl->setMarkdown( md );
		setShowDesc( !md.isEmpty() );
	}

	void Algo::setShowDesc( bool sd )
	{
		if ( showDesc != sd )
		{
			tb_expl->setVisible( sd );
			if ( ( showDesc = sd ) == true ) sd = false;
			tb_hideExpl->setVisible( sd );
			te_expl->setVisible( sd );
			emit showDescriptionChanged( showDesc );
		}
	}

	void Algo::setNPrio( int n )
	{
		sb_prio_cnt->setValue( n );
		tw_prio->setRowCount( n );
		auto vis = ( n > 0 );
		tw_prio->setVisible( vis );
		sb_prio_cnt->setVisible( vis );
	}

	void Algo::setShowAniCfg( bool sa )
	{
		if ( sa != showAniCfg )
		{
			gb_animation->setVisible( sa );
			emit showAnimationCfgChanged( showAniCfg = sa );
		}
	}

	void Algo::setProgressReporting( ProgressDisplay as )
	{
		if ( prgMode != as )
		{
			prgMode = as;
			( as == silent	 ? rb_silent
			  : as == visual ? rb_visual
							 : rb_animated )
				->setChecked( true );
		}
	}

	void Algo::on_sl_aniDur_valueChanged( int nms )
	{
		PositionAni::duration = nms;
	}

	void Algo::on_bg_visMode_idPressed( int button_id )
	{
		if ( prgMode != button_id ) // Der visuelle Modus ist nur für die UI interessant, kann bei
									// Änderung also sofort übernommen werden.
			prgMode = static_cast< ProgressDisplay >( button_id );
	}

	void Algo::on_bg_visMode_idReleased( int button_id )
	{
		// Inzwischen dürfte alle Information angekommen sein - wenn nicht, schon gerendert. Anyhow:
		// einmalig die Anzeige aktualisieren.
		newResults = true;
		idleProc->start();
	}

	// Really important content...
	void Algo::on_tb_expl_clicked()
	{
		tb_expl->hide();
		v->hide();
		tb_hideExpl->show();
		te_expl->show();
		masterGrid->setRowStretch( 0, 10 );
		masterGrid->setRowStretch( 2, 0 );
	}

	void Algo::on_tb_hideExpl_clicked()
	{
		tb_expl->show();
		v->show();
		tb_hideExpl->hide();
		te_expl->hide();
		masterGrid->setRowStretch( 0, 0 );
		masterGrid->setRowStretch( 2, 8 );
	}

	void Algo::on_pb_generate_clicked()
	{
		QRandomGenerator gen( QRandomGenerator::securelySeeded() );
		auto			 Ccnt = gen.bounded( sb_minCcnt->value(), sb_maxCcnt->value() );
		auto			 Pcnt = gen.bounded( sb_minBcnt->value(), sb_maxBcnt->value() );
		QString			 s;
		for ( int i = 0; i < Ccnt; i++ )
			s += ( s.isEmpty() ? "" : "," )
				 + QString::number( gen.bounded( sb_minSz->value(), sb_maxSz->value() ) );
		te_sizes->setPlainText( s );
		s.clear();
		for ( int i = 0; i < Pcnt; i++ )
			s += QString( "%1$%2" )
					 .arg( s.isEmpty() ? "" : "," )
					 .arg( ( sb_firstBlock->value() + i ) * 256
							   + gen.bounded( sb_minBoffs->value(), sb_maxBoffs->value() ),
						   4, 16, QChar( '0' ) );
		le_pages->setText( s );
		if ( v->scene() ) v->scene()->clear();
	}

	void Algo::on_pb_simulate_clicked()
	{
		if ( createRunner == nullptr ) return;
		if ( runnerThread )
		{ // Der Algo läuft schon - hier geht's jetzt um's Pausieren/Depausieren...
			emit do_pause(); // Die UI wird nach Rückmeldung im State angepasst!
		} else {
			runnerThread = new QThread( this );
			auto runner	 = createRunner( runnerCom );
			// Connect the init signal, so gui elements can be built in the foreground later...
			// Also bad input may trigger a final_solution of 0% -> needs a connect before init!
			connect( runner, &AlgoRunner::init_data_completed, this, &Algo::on_init_completed,
					 Qt::QueuedConnection );
			connect( runner, &AlgoRunner::final_solution, this, &Algo::on_final_solution,
					 Qt::QueuedConnection );
			// Initialize while still inside the GUI thread
			if ( runner->init( te_sizes->toPlainText(), le_pages->text() ) )
			{
				connect( runner, &AlgoRunner::state_changed, this, &Algo::on_state_changed,
						 Qt::QueuedConnection );
				connect( runner, &AlgoRunner::page_finished, this, &Algo::on_page_finished,
						 Qt::QueuedConnection );
				connect( this, &Algo::do_continue, runner, &AlgoRunner::iterate,
						 Qt::QueuedConnection );
				connect( this, &Algo::do_pause, runner, &AlgoRunner::pause, Qt::QueuedConnection );
				connect( runnerThread, &QThread::finished, this, &Algo::on_runnerThread_finished,
						 Qt::QueuedConnection );
				runner->moveToThread( runnerThread );
				runnerThread->start( QThread::LowPriority );
				pb_viscor->show();
			} else { // Maybe an error message would be nice ...
				delete runner;
				delete runnerThread, runnerThread = nullptr;
			}
			pb_generate->setEnabled( false );
		}
	}

	// Callbacks from the runner thread:
	// ==================================
	// -> create graphical representation and show it on screen
	void Algo::on_init_completed( QList< quint32 > _chunks, QMap< quint16, quint32 > _pages )
	{
		// Dieser Aufruf ist dafür da, eine Initialkonfiguration der graphischen Darstellung zu
		// generieren.
		auto sc = new QGraphicsScene( this );
		sc->addItem( gfx = new AlgoGfx( v->contentsRect().size(), _chunks, _pages ) );
		connect( gfx, &AlgoGfx::resized,
				 [ this ]()
				 {
					 v->fitInView( gfx, Qt::KeepAspectRatio );
					 qDebug() << "resized to fit in view ...";
				 } );
		v->setScene( sc );
		v->fitInView( gfx, Qt::KeepAspectRatio );
		emit do_continue( false );
		// Visual Correction Button einrichten: wird der Chunk-Mgr gelöscht, wird die erste
		// Verbindung automatisch gekappt.  Ich möchte aber, dass der Button dann auch versteckt
		// wird.
		connect( pb_viscor, &QPushButton::clicked,
				 [ this ]() { newResults = true, idleProc->start(); } );
		connect( gfx, &QObject::destroyed, pb_viscor, &QWidget::hide );
		pb_viscor->show();
	}

	void Algo::on_state_changed( Algo::State st )
	{
		if ( st != lastSt )
		{
			// Eine nette Nachricht für die Außenwelt basteln und emittieren
			bool isP = st & Algo::pause_bit;
			auto pg	 = currentPage( st );
			auto msg = st & bits_mask ? u"initialized and "_s : u"invalid"_s;
			if ( ( st & init_bit ) == 0 ) msg.prepend( "not " );
			if ( st & running_bit )
			{
				msg.append( "running" );
				if ( st & pause_bit ) msg.append( ", pause requested" );
			} else {
				if ( st & pause_bit ) msg.append( "paused" );
				else msg.append( "idle" );
			}
			if ( st & deeper_calc ) msg.append( ", trying to find alternate solutions" );
			msg = tr( "Algo is: %1 - currently @ page $%2 - iterated: %L3 times." )
					  .arg( msg )
					  .arg( pg, 4, 16, QChar{ '0' } )
					  .arg( currentStep( st ) );
			emit stateMessageChanged( msg );

			newResults |= ( currentPage( lastSt ) != pg );
			// Dies ist eine Rückmeldung über den Zustand der Berechnung
			if ( isP != bool( lastSt & Algo::pause_bit ) ) // -> Play-Pause-Button steuern!
				pb_simulate->setIcon(
					isP ? QIcon::fromTheme( QIcon::ThemeIcon::MediaPlaybackStart )
						: QIcon::fromTheme( QIcon::ThemeIcon::MediaPlaybackPause ) );
			lastSt = st;
		}
	}

	void Algo::on_page_finished()
	{
		// Benachrichtigung über die Lösung für eine Seite
		if ( !newResults )
		{
			newResults = true;
			if ( prgMode != silent ) idleProc->start();
		}
		// Ggf. angebracht - eine Meldung nach außen, dass eine Teillösung gefunden wurde?
	}

	void Algo::on_final_solution( quint64 iteration_count, quint64 selection_count,
								  quint64 deselection_count, qreal results_good )
	{
		auto msg =
			u"%1 -> %2% result quality after %L3 iterations with %L4/%L5 select and deselect operations."_s
				.arg( results_good > 0.75 ? "!!!SUCCESS!!!" : "...FAILURE..." )
				.arg( int( results_good * 100 ) )
				.arg( iteration_count )
				.arg( selection_count )
				.arg( deselection_count );
		emit stateMessageChanged( msg );
		disconnect( pb_simulate, &QPushButton::clicked, this, &Algo::do_continue );
		disconnect( pb_simulate, &QPushButton::clicked, this, &Algo::do_pause );
		runnerThread->quit();
		runnerThread->wait(); // make sure the thread has ended execution by now.
	}

	void Algo::on_runnerThread_finished()
	{
		qDebug() << "Algo::on_runnerThread_finished()";
		// restore UI after finishing
		pb_simulate->setIcon( QIcon::fromTheme( QIcon::ThemeIcon::Computer ) );
		pb_generate->setEnabled( true );
		runnerThread->deleteLater();
		runnerThread = nullptr;
		gfx->updateState( runnerCom.result, true );
		v->fitInView( gfx, Qt::KeepAspectRatio );
	}

	void Algo::idle_processing()
	{
		if ( PositionAni::running < 1 && newResults )
		{
			idleProc->stop();
			newResults = false;
			if ( gfx )
			{
				runnerCom.lock.lockForRead();
				gfx->updateState( runnerCom.result,
								  ( prgMode == animated ) || ( ( lastSt & running_bit ) == 0 ) );
				runnerCom.lock.unlock();
				runnerCom.lock.lockForWrite();
				runnerCom.freshData = false;
				runnerCom.lock.unlock();
			}
		}
	}


	AlgoRunner::AlgoRunner( AlgoCom &communication_structure )
		: QObject()
		, com( communication_structure )
		, current_state( Algo::pause_bit )
	{
		com.freshData = false;
		com.result.clear();
		// Interne Verbindung, um den Stack beim Backtracking zu schonen.  Hier wird im Endeffekt
		// schliesslich eine mehrstufige Rekursion iterativ aufgedröselt ...
		connect( this, &AlgoRunner::redo, this, &AlgoRunner::pre_re_iterate, Qt::QueuedConnection );
	}

	bool AlgoRunner::init( QString chunk_sizes, QString page_descriptions, QStringList priorities )
	{
		// Prioritäten zu Zahlen machen
		QList< QList< int > > ps;
		for ( auto p : priorities )
		{
			ps.append( QList< int >{} );
			for ( auto pi : p.split( ',' ) ) ps.last().append( pi.toInt() );
		}
		// chunks einlesen und sortieren:
		for ( auto sz : chunk_sizes.split( ',' ) )
		{
			auto p = 0; // Prio bestimmen
			while ( p < ps.count() && !ps[ p ].contains( chunks.count() ) ) ++p;
			sort_into_avail( chunks.insert( chunks.count(), read_maybehex( sz ) | ( p << 16 ) )
							 - chunks.begin() );
		}
		// pages einlesen und sortieren (Sortierung durch die QMap via integer-key):
		auto &pages = com.result;
		for ( auto ptx : page_descriptions.split( ',' ) )
		{
			auto pi = read_page_info( ptx );
			auto it = pages.insert( pi.first,
									{ pi.first,
									  quint16( pi.first + pi.second & 0xffffu ),
									  quint16( pi.second & 0xffff ),
									  quint16( 0 ),
									  {},
									  {} } );
			// CHECK ARGUMENTS! Neither >prev, nor >next shall be the same as >pi.first
			// Die gleiche Page soll nicht mehrmals bearbeitet werden
			if ( ( it != pages.begin() && ( ( it - 1 ).key() >> 8 ) >= ( pi.first >> 8 ) )
				 || ( it + 1 != pages.end() && ( ( it + 1 ).key() >> 8 ) <= ( pi.first >> 8 ) ) )
				pages.remove( it.key() );
			else
			{
				// Bearbeitungsreihenfolge einsortieren
				p_id = 0;
				while ( p_id < pageOrder.count()
						&& ( ( pi.second > pages[ pageOrder[ p_id ] ].bytes_left )
							 || ( ( pi.second == pages[ pageOrder[ p_id ] ].bytes_left )
								  && ( pi.first > pages[ pageOrder[ p_id ] ].start_address ) ) ) )
					p_id++;
				pageOrder.insert( p_id, pi.first );
			}
		}
		// so it seems we have the init completed - time to send initial data to the user interface
		QMap< quint16, quint32 > icd;
		for ( const auto &p : pages )
			icd.insert( p.start_address, p.bytes_left | ( quint32( p.end_address ) << 16 ) );
		emit init_data_completed( chunks, icd );
		// initialize engine state, so any further start-stop-whatever-calls just work right.
		a_id = p_id = cnt_sel = cnt_unsel = iteration = 0;
		current_state								  = Algo::init_bit;
		if ( !( chunks.count() && pages.count() ) )
		{ // Wenn wirklich kein Initialzustand gefunden werden kann - beende die Engine komplett!
			emit final_solution( iteration, cnt_sel, cnt_unsel, 0.f );
			return false;
		} else return true;
	}

	void AlgoRunner::pause()
	{
		if ( current_state & Algo::pause_bit ) // Pause wurde schon requested ...
		{
			if ( ( current_state & Algo::running_bit ) == 0 ) // Request wurde angenommen
			{
				current_state &= ~Algo::pause_bit;
				pre_re_iterate();
			}
		} else current_state |= Algo::pause_bit;
		qDebug() << "AlgoRunner::pause() completed.";
	}

	void AlgoRunner::pre_re_iterate()
	{
		// Diese Funktion ist sozusagen die große While-Schleife drum herum, die den Status des
		// Threads prüft und ggf. die Handlung unterbricht und stateChanged meldet
		// Setze aktuellen Zustand neu     -> TODO!
		bool report =
			( lastIt >> 14 )
			!= ( iteration >> 14 ); // 16k Iterationen zwischen Meldungen sind ausreichend!

		if ( current_state & Algo::pause_bit ) // Pause angefragt?
		{
			current_state &= ~Algo::running_bit; // signal we're not running anymore
			report = true;
		} else current_state |= Algo::running_bit;

		if ( report )
			emit state_changed( current_state =
									Algo::fromValues( current_state & Algo::bits_mask,
													  pageOrder[ p_id ], lastIt = iteration ) );
		if ( ( current_state & Algo::pause_bit ) == 0 ) iterate();
		else qDebug() << "AlgoRunner::pre_re_iterate(): Pause accepted.";
	}

	void AlgoRunner::sort_into_avail( int chunk_id )
	{
		assert( chunk_id < chunks.count() );
		a_id = 0;
		while ( a_id < avail.count()
				&& ( bytes( chunk_id ) < bytes( avail[ a_id ] )
					 || ( bytes( chunk_id ) == bytes( avail[ a_id ] )
						  && ( prio( avail[ a_id ] ) < prio( chunk_id )
							   || prio( avail[ a_id ] ) == prio( chunk_id )
									  && avail[ a_id ] < chunk_id ) ) ) )
			++a_id;
		avail.insert( a_id++, chunk_id );
	}

	// returns true if the current page id changed somehow.
	bool AlgoRunner::take_available()
	{
		assert( a_id < avail.count() && p_id < pageOrder.count() );
		++cnt_sel;
		auto id = avail.takeAt( a_id );
		com.result[ pageOrder[ p_id ] ].selection.append( id );
		if ( ( com.result[ pageOrder[ p_id ] ].bytes_left -= bytes( id ) ) <= cur_btsleft_thresh )
		{
			// Page finished - jetzt haben wir eine neue best solution
			QWriteLocker lck( &com.lock );
			com.result[ pageOrder[ p_id ] ].solution = com.result[ pageOrder[ p_id ] ].selection;
			if ( !com.freshData )
			{
				com.freshData = true;
				emit page_finished();
			}
			++p_id, a_id = 0;
			return true;
		} else return false;
	}

	// returns true if the current page id changed somehow.
	bool AlgoRunner::make_available()
	{
		assert( p_id < pageOrder.count() );
		++cnt_unsel;
		if ( com.result[ pageOrder[ p_id ] ].selection.isEmpty() )
		{
			// page backtracking needs to happen:
			// -> p_id == 0 -> smallest page? Cannot decrement -> becomes bad, cannot fill it.
			// -> else --p_id
			//  -> both should also send a signal, as the current page changes, thus a page_solution
			//  has changed for now.
			QWriteLocker lck( &com.lock );
			com.result[ pageOrder[ p_id ] ].solution.clear();
			if ( p_id > 0 ) --p_id; // go backtrack the previous page ...
			else
			{ // mark page as not considered anymore
				com.result[ pageOrder[ p_id ] ].selection.append( -1 );
				badPages.append( pageOrder.takeFirst() ), a_id = 0; // ... start again ...
			}
			if ( !com.freshData )
			{
				com.freshData = true;
				emit page_finished();
			}
			return true; // return true to signal end of inner loop, as a page jump happened
		}
		auto c_id = com.result[ pageOrder[ p_id ] ].selection.takeLast();
		com.result[ pageOrder[ p_id ] ].bytes_left += bytes( c_id );
		sort_into_avail( c_id );
		return false;
	}

	QPair< quint16, quint16 > AlgoRunner::read_page_info( const QString tx ) const
	{
		auto tl = tx.split( '+' );
		auto sa = read_maybehex( tl.first() );
		return { sa, tl.count() > 1 ? read_maybehex( tl.last() ) : ( ( sa + 256 ) & 0xff00 ) - sa };
	}

	quint16 AlgoRunner::read_maybehex( const QString tx ) const
	{
		auto tt	 = tx.trimmed();
		int	 hex = tt.startsWith( u'$' ) ? 1 : tt.startsWith( u"0x", Qt::CaseInsensitive ) ? 2 : 0;
		if ( hex ) return tt.mid( hex, -1 ).toUInt( nullptr, 16 );
		else return tt.toUInt();
	}


	// Was soll diese iterate() - Variante genau machen?
	//
	//  -> klar im Eingang: die Finale Lösung liefern, wenn es soweit ist
	//  -> aktuellen Status setzen - eine weitere "Großiteration"
	//  -> kleine Schleife: Page zu füllen versuchen
	//      -> hier erweitert um: Ergebnis von "make_available"
	//
	void AlgoEngine::iterate()
	{
		++iteration;
		do { // Außen: Backtracking, Innen: Number Selecting
			while ( a_id < avail.count()
					&& com.result[ pageOrder[ p_id ] ].bytes_left > cur_btsleft_thresh )
				if ( com.result[ pageOrder[ p_id ] ].bytes_left >= chunks[ avail[ a_id ] ] )
				{
					if ( take_available() )
						if ( p_id < pageOrder.count() )
							return emit redo(); // Nächste Page -> neue Iteration!
						else return iterate_complete();
				} else ++a_id;
		} while ( !avail.isEmpty() && !make_available() );

		if ( avail.isEmpty() )
		{
			emit page_finished();
			emit final_solution( iteration, cnt_sel, cnt_unsel, 1.0 );
		} else {
			// Wir sind also nicht fertig geworden, keine Pause angefragt -> das heisst das
			// Backtracking hat einen Seitensprung gemacht.
			if ( pageOrder.isEmpty() || p_id >= pageOrder.count() )
				emit final_solution( iteration, cnt_sel, cnt_unsel, avail.isEmpty() );
			else emit redo();
		}
	}

	void AlgoEngine::iterate_complete()
	{
		// Die "final Solution" muss unvollständig sein, weil ja noch Chunks
		// available sind.  Hier ist noch kein "deeper shit" passiert, ich gebe
		// erst einmal eine Teillösung aus.  Muss ich natürlich berechnen,
		// wieviel Prozent gelöst sind ...
		int all = 0, left = 0;
		for ( int id = 0; id < chunks.count(); id++ )
		{
			all += chunks[ id ];
			if ( avail.contains( id ) ) left += chunks[ id ];
		}
		// Prozentual: wie viele Bytes sind unter der Haube?
		auto p_c = 1. - ( qreal( left ) / qreal( all ) );
		// Allerdings gäbe es noch andere Kriterien: Wieviele Chunks sind gefüllt?
		left = all = 0;
		for ( auto p : com.result ) // nicht befüllbare Pages nicht zählen!
			if ( !( p.selection.count() == 1 && p.selection.first() == -1 ) )
			{
				left += p.bytes_left;
				all += p.end_address - p.start_address;
			}
		auto		p_p = 1. - ( qreal( left ) / qreal( all ) );
		return emit final_solution( iteration, cnt_sel, cnt_unsel, ( p_c + p_p ) * .5 );
	}
} // namespace Algo
