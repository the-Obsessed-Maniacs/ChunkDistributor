#include "Algo.h"

#include "AlgoGfx.h"

#include <QChronoTimer>
#include <QRandomGenerator>
#include <QThread>

namespace Algo
{
	const AlgoPage &WorkerBase::solution( quint16 page_addr ) const
	{
		auto it = pages.find( page_addr );
		if ( it == pages.end() )
		{
			static AlgoPage empty;
			return empty;
		}
		return *it;
	}
	void WorkerBase::setInitData( ChunkList c, PageList p )
	{
		chunks.swap( c );
		while ( p.count() )
		{
			const auto sa = p.first().start_addr;
			pages[ sa ]	  = { p.takeFirst() };
		}
		// So chunks and pages are filled.  Leaves me to produce the sorted lists.
		avail.clear(), pageOrder.clear(), badPages.clear();
		for ( int id = 0; id < chunks.count(); id++ ) sort_into_avail( id );
		for ( auto id : pages.keys() )
		{
			// Bearbeitungsreihenfolge einsortieren
			p_id = 0;
			while (
				p_id < pageOrder.count()
				&& ( ( pages[ id ]._.bytes_left > pages[ pageOrder[ p_id ] ]._.bytes_left )
					 || ( ( pages[ id ]._.bytes_left == pages[ pageOrder[ p_id ] ]._.bytes_left )
						  && ( pages[ id ]._.start_addr
							   > pages[ pageOrder[ p_id ] ]._.start_addr ) ) ) )
				p_id++;
			pageOrder.insert( p_id, id );
		}
		p_id = a_id = iteration = cnt_sel = cnt_unsel = lastIt = cur_btsleft_thresh = 0;
		currentPage = &pages[ pageOrder[ p_id ] ];
		connect( this, &WorkerBase::redo, this, &WorkerBase::pre_re_iterate, Qt::QueuedConnection );
		emit state_changed( current_state = State::init_bit );
	}
	void WorkerBase::pause()
	{
		if ( current_state & State::pause_bit ) // Pause wurde schon requested ...
		{
			if ( ( current_state & State::running_bit ) == 0 ) // Request wurde angenommen
			{
				current_state &= ~State::pause_bit;
				pre_re_iterate();
			}
		} else current_state |= State::pause_bit;
		qDebug() << "AlgoRunner::pause() completed.";
	}
	void WorkerBase::sort_into_avail( int chunk_id )
	{
		assert( chunk_id < chunks.count() );
		const auto &c = chunks[ chunk_id ];
		a_id		  = 0;
		while ( a_id < avail.count()
				&& ( c.size < chunks[ avail[ a_id ] ].size
					 || ( c.size == chunks[ avail[ a_id ] ].size
						  && ( chunks[ avail[ a_id ] ].prio < c.prio
							   || chunks[ avail[ a_id ] ].prio == c.prio
									  && avail[ a_id ] < chunk_id ) ) ) )
			++a_id;
		avail.insert( a_id++, chunk_id );
	}
	// Take Available returns true when a page got finished
	bool WorkerBase::take_available()
	{
		assert( a_id < avail.count() && p_id < pageOrder.count() );
		++cnt_sel;
		const auto	id	= avail.takeAt( a_id );
		const auto &c	= chunks[ id ];
		auto	   &pcp = *currentPage;
		pcp.selection.append( id );
		if ( ( pcp._.bytes_left -= c.size ) <= cur_btsleft_thresh )
		{
			// Page finished - jetzt haben wir eine neue best solution
			QWriteLocker lck( &lock );
			pcp.solution = pcp.selection;
			emit page_finished( pcp._.start_addr );
			++p_id, a_id = 0;
			currentPage = ( p_id < pageOrder.count() ) ? &pages[ pageOrder[ p_id ] ] : nullptr;
			return true;
		} else return false;
	}
	// Make Available returns true, when backtracking dropped the current page.
	bool WorkerBase::make_available()
	{
		assert( p_id < pageOrder.count() );
		++cnt_unsel;
		auto &pcp = *currentPage;
		if ( pcp.selection.isEmpty() )
		{
			// page backtracking needs to happen:
			// -> p_id == 0 -> smallest page? Cannot decrement -> becomes bad, cannot fill it.
			// -> else --p_id
			QWriteLocker lck( &lock );
			pcp.solution.clear();
			if ( p_id > 0 ) --p_id; // go backtrack the previous page ...
			else
			{ // mark page as not considered anymore
				pcp.selection.emplace_back( -1 );
				badPages.emplace_back( pageOrder.takeFirst() ), a_id = 0; // ... start again ...
			}
			currentPage = &pages[ pageOrder[ p_id ] ];
			emit page_finished( pcp._.start_addr );
			return true; // return true to signal end of inner loop, as a page jump happened
		}
		auto c_id = pcp.selection.takeLast();
		pcp._.bytes_left += chunks[ c_id ].size;
		sort_into_avail( c_id );
		return false;
	}
	// handles actual pausing
	void WorkerBase::pre_re_iterate()
	{ // Diese Funktion ist sozusagen die große While-Schleife drum herum, die den Status des
		// Threads prüft und ggf. die Handlung unterbricht und stateChanged meldet
		// Setze aktuellen Zustand neu     -> TODO!
		bool report =
			( lastIt >> 14 )
			!= ( iteration >> 14 ); // 16k Iterationen zwischen Meldungen sind ausreichend!

		if ( current_state & State::pause_bit ) // Pause angefragt?
		{
			current_state &= ~State::running_bit; // signal we're not running anymore
			report = true;
		} else current_state |= State::running_bit;

		if ( report )
			emit state_changed( current_state =
									fromValues( current_state & State::bits_mask, pageOrder[ p_id ],
												lastIt = iteration ) );
		if ( ( current_state & State::pause_bit ) == 0 ) iterate();
		else qDebug() << "AlgoRunner::pre_re_iterate(): Pause accepted.";
	}

	Algorithm::Algorithm( QWidget *pw )
		: QWidget( pw )
		, idleProc( new QChronoTimer( this ) )
		, thread( new QThread )
	{
		setupUi( this );
		setNPrio( sb_prio_cnt->value() );
		// Die Erklärungsanzeige ist programmiert ;)
		// Weiter geht es ... die Zufallsgeneration implementieren
		for ( auto [ i, a ] : { QPair{ sb_minSz, sb_maxSz },
								{ sb_minCcnt, sb_maxCcnt },
								{ sb_minBoffs, sb_maxBoffs },
								{ sb_minBcnt, sb_maxBcnt } } )
			connect( i, &QSpinBox::valueChanged, a, &QSpinBox::setMinimum ),
				connect( a, &QSpinBox::valueChanged, i, &QSpinBox::setMaximum );
		connect( sb_prio_cnt, &QSpinBox::valueChanged, this, &Algorithm::setNPrio );
		// Animation-Display: die Button-Group braucht noch Informationen, die im Designer nicht
		// gesetzt werden können.
		bg_visMode->setId( rb_silent, silent );
		bg_visMode->setId( rb_visual, visual );
		bg_visMode->setId( rb_animated, animated );
		on_tb_hideExpl_clicked();
		// Und das Viewport-Wheel-Zooming ...
		v->viewport()->installEventFilter( this );
		v->setMouseTracking( true );
		cb_algo->addItems( AlgoWorker::keys() );
		// Idle-Processing: wenn der Rechenthread zu schnell ist und die MessageQueue befüllt, kann
		// nicht Animiert werden.  Das heisst: die gemeldeten Rechenergebnisse werden lediglich
		// zwischengespeichert, ein Flag gesetzt und soll später bei "idle" durchgeführt werden.
		connect( idleProc, &QChronoTimer::timeout, this, &Algorithm::idleProcessing );
	}
	Algorithm::~Algorithm()
	{
		if ( thread ) thread->quit(), thread->wait(), delete thread;
	}
	bool Algorithm::eventFilter( QObject *o, QEvent *e )
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
	void Algorithm::clearWorker()
	{ // If a previous run existed, remove the old worker!
		if ( worker ) delete worker, worker = nullptr;
	}
	// Stuff needed for the Designer implementation
	void Algorithm::setDesc( QString md )
	{
		te_expl->setMarkdown( md );
		setShowDesc( !md.isEmpty() );
	}
	void Algorithm::setShowDesc( bool sd )
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
	void Algorithm::setNPrio( int n )
	{
		sb_prio_cnt->setValue( n );
		tw_prio->setRowCount( n );
		auto vis = ( n > 0 );
		tw_prio->setVisible( vis );
		sb_prio_cnt->setVisible( vis );
	}
	void Algorithm::setShowAniCfg( bool sa )
	{
		if ( sa != showAniCfg )
		{
			gb_animation->setVisible( sa );
			emit showAnimationCfgChanged( showAniCfg = sa );
		}
	}
	void Algorithm::setProgressReporting( ProgressDisplay as )
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
	void Algorithm::on_sl_aniDur_valueChanged( int nms )
	{
		PositionAni::duration = nms;
	}
	void Algorithm::on_bg_visMode_idPressed( int button_id )
	{
		if ( prgMode != button_id ) // Der visuelle Modus ist nur für die UI interessant, kann bei
									// Änderung also sofort übernommen werden.
			prgMode = static_cast< ProgressDisplay >( button_id );
	}
	void Algorithm::on_bg_visMode_idReleased( int button_id )
	{
		// Inzwischen dürfte alle Information angekommen sein - wenn nicht, schon gerendert. Anyhow:
		// einmalig die Anzeige aktualisieren.
		idleProc->start();
	}
	void Algorithm::on_tb_expl_clicked()
	{
		tb_expl->hide();
		v->hide();
		tb_hideExpl->show();
		te_expl->show();
		masterGrid->setRowStretch( 0, 10 );
		masterGrid->setRowStretch( 2, 0 );
	}
	void Algorithm::on_tb_hideExpl_clicked()
	{
		tb_expl->show();
		v->show();
		tb_hideExpl->hide();
		te_expl->hide();
		masterGrid->setRowStretch( 0, 0 );
		masterGrid->setRowStretch( 2, 8 );
	}
	void Algorithm::on_pb_generate_clicked()
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
		clearWorker();
	}
	void Algorithm::on_pb_simulate_clicked()
	{
		if ( cb_algo->currentIndex() == -1 && worker == nullptr ) return;
		if ( worker && !pb_generate->isEnabled() )
		{ // Der Algo läuft schon - hier geht's jetzt um's Pausieren/Depausieren...
			emit do_pause(); // Die UI wird nach Rückmeldung im State angepasst!
		} else {
			ChunkList			  chunks;
			PageList			  pages;
			QList< QList< int > > ps;
			// Prioritäten zu Zahlen machen
			for ( int i = 0; i < tw_prio->rowCount(); ++i )
			{
				auto p = tw_prio->item( i, 1 )->data( Qt::DisplayRole ).toString();
				ps.append( QList< int >{} );
				for ( auto pi : p.split( ',' ) ) ps.last().append( read_maybehex( pi ) );
			}
			// chunks einlesen:
			for ( auto sz : te_sizes->toPlainText().split( ',' ) )
			{
				quint16 p = 0; // Prio bestimmen
				while ( p < ps.count() && !ps[ p ].contains( chunks.count() ) ) ++p;
				chunks.emplaceBack( read_maybehex( sz ), p );
			}
			for ( auto pg : le_pages->text().split( ',' ) )
			{
				auto pi	 = read_page_info( pg );
				auto ph	 = pi.first >> 8;
				// nicht die gleichen Pages mehrfach haben ...
				auto end = std::find_if( pages.begin(), pages.end(), [ ph ]( const PageData &pd )
										 { return ( pd.start_addr >> 8 ) == ph; } );
				if ( end == pages.end() )
					pages.emplaceBack( pi.first, pi.second
													 ? pi.first + pi.second
													 : ( pi.first + 256 ) & 0xff00 - pi.first );
			}
			// Abbruch ohne Daten!
			if ( chunks.isEmpty() || pages.isEmpty() ) return;
			// UI initialisieren
			auto sc = new QGraphicsScene( this );
			sc->addItem( gfx = new AlgoGfx( v->contentsRect().size(), chunks, pages ) );
			v->setScene( sc );
			gfx->performLayout( true );
			v->fitInView( gfx, Qt::KeepAspectRatio );
			connect( gfx, &AlgoGfx::resized,
					 [ this ]()
					 {
						 v->fitInView( gfx, Qt::KeepAspectRatio );
						 qDebug() << "resized to fit in view ...";
					 } );
			// Worker initialisieren und starten
			if ( worker ) worker->deleteLater(), worker = nullptr;
			if ( worker = AlgoWorker::make( cb_algo->currentText() ) )
			{
				worker->setInitData( chunks, pages );
				worker->moveToThread( thread );
				connect( worker, &WorkerBase::final_solution, this, &Algorithm::finalSolution );
				connect( worker, &WorkerBase::state_changed, this, &Algorithm::stateChange );
				connect( worker, &WorkerBase::page_finished, this, &Algorithm::pageFinished );
				connect( this, &Algorithm::do_continue, worker, &WorkerBase::iterate );
				connect( this, &Algorithm::do_pause, worker, &WorkerBase::pause );
				if ( !thread->isRunning() ) thread->start( QThread::HighestPriority );
				pb_generate->setEnabled( false );
				cb_algo->setEnabled( false );
				emit do_continue();
			}
		}
	}
	void Algorithm::stateChange( State st )
	{
		if ( st != lastSt )
		{
			// Eine nette Nachricht für die Außenwelt basteln und emittieren
			bool isP = st & pause_bit;
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

			// Dies ist eine Rückmeldung über den Zustand der Berechnung
			if ( isP != bool( lastSt & pause_bit ) ) // -> Play-Pause-Button steuern!
				pb_simulate->setIcon(
					isP ? QIcon::fromTheme( QIcon::ThemeIcon::MediaPlaybackStart )
						: QIcon::fromTheme( QIcon::ThemeIcon::MediaPlaybackPause ) );
			lastSt = st;
		}
	}
	void Algorithm::pageFinished( quint16 page_id )
	{
		updateRequests.insert( page_id );
		if ( prgMode != silent && !idleProc->isActive() ) idleProc->start();
	}
	void Algorithm::finalSolution( quint64 iteration_count, quint64 selection_count,
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
		disconnect( pb_simulate, &QPushButton::clicked, this, &Algorithm::do_pause );
		pb_generate->setEnabled( true );
		cb_algo->setEnabled( true );
	}
	void Algorithm::idleProcessing()
	{
		if ( PositionAni::running < 1 && !updateRequests.isEmpty() )
		{
			idleProc->stop();
			if ( gfx )
			{
				auto locker = worker->obtainReadAccess();
				for ( auto it = updateRequests.begin(); it != updateRequests.end(); )
				{ // Update the corresponding pages ...
					auto id = *it;
					gfx->changePage( worker->solution( *it ) );
					it = updateRequests.erase( it );
				}
				gfx->commitPages( ( prgMode == animated ) || ( ( lastSt & running_bit ) == 0 ) );
			} // And the lock should be released, as locker is destroyed ...
		}
	}
	QPair< quint16, quint16 > Algorithm::read_page_info( const QString tx ) const
	{
		auto tl = tx.split( '+' );
		auto sa = read_maybehex( tl.first() );
		return { sa, tl.count() > 1 ? read_maybehex( tl.last() ) : ( ( sa + 256 ) & 0xff00 ) - sa };
	}
	quint16 Algorithm::read_maybehex( const QString tx ) const
	{
		auto tt	 = tx.trimmed();
		int	 hex = tt.startsWith( u'$' ) ? 1 : tt.startsWith( u"0x", Qt::CaseInsensitive ) ? 2 : 0;
		if ( hex ) return tt.mid( hex, -1 ).toUInt( nullptr, 16 );
		else return tt.toUInt();
	}
} // namespace Algo
