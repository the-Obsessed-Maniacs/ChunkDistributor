
#include "AlgoGfx.h"

#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace PositionAni
{
	int	 running = 0, paused = 0, duration = 250;

	void update_state( QAbstractAnimation::State ns, QAbstractAnimation::State os )
	{
		switch ( os )
		{
			case QAbstractAnimation::Stopped:
				switch ( ns )
				{
					case QAbstractAnimation::Paused: ++paused;
					case QAbstractAnimation::Running: ++running; break;
				}
				break;
			case QAbstractAnimation::Paused:
				switch ( ns )
				{
					case QAbstractAnimation::Stopped: --running;
					case QAbstractAnimation::Running: --paused; break;
				}
				break;
			case QAbstractAnimation::Running:
				switch ( ns )
				{
					case QAbstractAnimation::Stopped: --running; break;
					case QAbstractAnimation::Paused: ++paused; break;
				}
				break;
		}
	}
} // namespace PositionAni

namespace Algo
{
	Chunk::Chunk( QGraphicsWidget *parent, uint bytes, uint id, int prio )
		: QGraphicsObject( parent )
		, PositionAni::Mated< Chunk >()
		, _bytes( bytes )
		, _id( id )
		, _prio( prio )
		, _w( bytePixels * bytes )
		, _myText( u"Chunk #%1 - %2 Bytes"_s.arg( id, 2, 16, QChar{ '0' } ).arg( bytes ) )
	{
		auto fm = QFontMetricsF( parent->font() );
		auto tw = _w - 2 * hm();
		if ( fm.horizontalAdvance( _myText ) >= tw )
			_myText = u"#%1 - %2"_s.arg( id, 2, 16, QChar{ '0' } ).arg( bytes );
		if ( fm.horizontalAdvance( _myText ) >= tw )
			_myText = u"%1"_s.arg( id, 2, 16, QChar{ '0' } );
		if ( fm.horizontalAdvance( _myText ) >= tw ) _myText = u"%1"_s.arg( id, 0, 16 );
		setZValue( 1. );
	}

	QRectF Chunk::boundingRect() const
	{
		return QRectF( 0, 0, _w, fh() );
	}

	void Chunk::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		QPainterPath pp;
		pp.addRoundedRect( option->rect, 5, 5 );
		painter->strokePath( pp, painter->pen() );
		painter->fillPath( pp, QColor::fromHslF( 0.3333f * ( 2 - _prio ), .7f, .72f ) );
		painter->setPen( QColor::fromHslF( 1.0f - ( 0.3f * _prio ), .7f, .72f ).darker() );
		painter->drawText( option->rect, Qt::AlignCenter | Qt::AlignJustify, _myText );
	}

	Page::Page( QGraphicsWidget *parent, uint start_address, uint end_address )
		: QGraphicsObject( parent )
		, PositionAni::Mated< Page >()
		, _start_address( start_address )
		, _end_address( end_address )
		, _leftNow( end_address - start_address )
		, _myText( u"%3 - start: $%1, ($%2=>%4)"_s.arg( start_address, 4, 16, QChar{ '0' } )
					   .arg( end_address - start_address, 2, 16, QChar{ '0' } )
					   .arg( start_address & ~0xffu, 2, 16, QChar{ '0' } )
					   .arg( end_address - start_address ) )
		, _myState( none )
	{
		setZValue( .5 );
	}

	void Page::setCurrentResult( QString content_string, int content_size, Status changed_state )
	{ // Hierher kann die Textbreiten-Berechnung und eigentlich auch die Farb-Berechnung ausgelagert
	  // werden.  Damit ist das Page-Rendern auch etwas leichtgewichtiger ...
		_leftNow   = _end_address - _start_address - content_size;
		_myLeft	   = u"to go: %1"_s.arg( _leftNow, 3 );
		_myContent = content_string;
		if ( auto pw = parentWidget() )
		{
			auto fm	   = QFontMetricsF( pw->font() );
			auto space = 256 * bs() - fm.horizontalAdvance( _myText )
						 - fm.horizontalAdvance( _myLeft ) - 2 * hs();
			if ( space < 2 * hs() ) _myContent.clear();
			else if ( fm.horizontalAdvance( content_string ) > space )
				_myContent = fm.elidedText( content_string, Qt::ElideMiddle, space );
		}
		// Farben: da könnte ich mir einen static const Gradient machen ...
		// - Grundgröße: bl (bytes left)
		// Werte: -1     0       8        16      ...256
		//      tiefrot  grün    türkis   gelb    immer röter werdend
		const auto drtl = ( 1.f / 3.f ), gut = 8.f, hmm = 16.f;
		if ( _leftNow < 0 ) _contCol = Qt::red;
		else if ( _leftNow <= gut )
			_contCol = QColor::fromHslF(
				qMax( qMin( drtl + ( ( 0.5f - drtl ) / gut * _leftNow ), 0.999999f ), 0.f ),
				qMax( qMin( .8f - ( 0.08f / gut ) * _leftNow, 0.999999f ), 0.f ), .70f );
		else if ( _leftNow <= hmm )
			_contCol = QColor::fromRgb(
				qRound( 123 + ( 234 - 123 ) * qMax( qMin( ( _leftNow - hmm ) / hmm, 1.f ), 0.f ) ),
				234,
				qRound( 228
						- ( 228 - 123 ) * qMax( qMin( ( _leftNow - hmm ) / hmm, 1.f ), 0.f ) ) );
		else
			_contCol = QColor::fromHslF(
				.5f * drtl * ( 1.f - qMin( _leftNow - hmm, hmm ) / hmm ),
				( 0.999999f - ( ( _leftNow - hmm ) / ( _end_address - _start_address - hmm ) ) )
						* .27f
					+ .72f,
				.70f );
		if ( changed_state != -1 ) _myState = changed_state;
		update();
	}

	QRectF Page::boundingRect() const
	{
		return QRectF( 0., 0., bytePixelsF * 256 + 2 * hm(), 2 * ( fh() + vm() ) + vs() );
	}

	void Page::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		auto		 r	= option->rect.toRectF();
		auto		 cr = r.adjusted( hm(), vm() + fh() + vs(), -hm(), -vm() );
		QPainterPath pp;
		// Outline + Background
		pp.addRoundedRect( r, 2 * bs(), 2 * bs() );
		painter->fillPath( pp, option->palette.brush( QPalette::Base ) );
		auto pen =
			_myState == none
				? painter->pen()
				: QColor::fromHslF( _myState == finished ? 216.f / 360.f : 33.f / 36.f, .72f, .7f );
		painter->strokePath( pp, pen );
		pp.clear();
		// Chunk-Background - nur der auffüllbare Platz sollte die AB-Farbe bekommen, die Outline
		// kommt zuletzt
		pp.addRoundedRect( cr.adjusted( ( _start_address & 0xff ) * bs() - 1, -1,
										1 - ( 0x100 - ( _end_address & 0xff ) ) * bs(), 1 ),
						   bs() + .5, bs() + .5 );
		painter->fillPath( pp, option->palette.brush( QPalette::AlternateBase ) );
		// Outline
		pp.clear();
		pp.addRoundedRect( cr.adjusted( -2, -2, 2, 2 ), bs() + 1, bs() + 1 );
		painter->strokePath( pp, option->palette.color( QPalette::Midlight ) );
		// Text ausgeben ...
		// ... erstmal das Rect beschränken -> beide Seiten und Top einrücken, danach Höhe setzen
		r.adjust( hm(), vm(), -hm(), 0 );
		r.setHeight( fh() );
		painter->drawText( r, Qt::AlignLeft | Qt::AlignVCenter, _myText, &cr );
		r.setLeft( cr.right() + hs() );
		painter->drawText( r, Qt::AlignRight | Qt::AlignVCenter, _myLeft, &cr );
		if ( !_myContent.isEmpty() )
		{
			r.setRight( cr.left() - hs() );
			r = r.normalized();
			painter->setPen( _contCol );
			painter->drawText( r, Qt::AlignCenter, _myContent );
		}
	}


	int AlgoGfx::hm, AlgoGfx::vm, AlgoGfx::hs, AlgoGfx::vs, AlgoGfx::fh;

	AlgoGfx::AlgoGfx( const QSize &currentViewSize, const QList< quint32 > &chunks,
					  const QMap< quint16, quint32 > &pages )
		: QGraphicsWidget( nullptr )
		, _cols( 1 )
		, _sbs( true )
	{
		setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::Frame );
		hm				 = style()->pixelMetric( QStyle::PM_LayoutLeftMargin );
		hs				 = style()->pixelMetric( QStyle::PM_LayoutHorizontalSpacing );
		vm				 = style()->pixelMetric( QStyle::PM_LayoutBottomMargin );
		vs				 = style()->pixelMetric( QStyle::PM_LayoutVerticalSpacing );
		fh				 = QFontMetrics( font() ).height();
		auto s			 = currentViewSize.toSizeF();
		_sizeRatioTarget = s.width() / s.height();
		// Initialkonfiguration der Elemente erstellen.  Zumindest für Pages sollte es noch eine Art
		// "Nachproduktionsmöglichkeit" geben...
		for ( const auto &[ addr, enda_size ] : pages.asKeyValueRange() )
			addNewPage( addr, enda_size, false );

		auto cnt = chunks.count();
		_chunks.resizeForOverwrite( cnt ), _chunk_pos.resize( cnt, { 0.f, 0.f, 0.f, -1.f } );
		for ( uint c_id = 0; c_id < cnt; c_id++ )
			_chunks[ c_id ] =
				new Chunk( this, chunks[ c_id ] & 0xffffu, c_id, chunks[ c_id ] >> 16 );

		updateMyGeometry();

		qDebug() << "AlgoGfx::CTor(" << this << ") done - got:" << _chunks.count()
				 << "chunks (positions:" << _chunk_pos.count() << "), and" << _pages.count()
				 << "pages (positions:" << _page_pos.count() << ")";
	}

	Page *AlgoGfx::addNewPage( uint address, uint enda_size, bool single_call )
	{
		auto page = new Page( this, address, enda_size >> 16 );
		_pages.insert( address, page );
		_page_pos.insert( address, QPointF{} );
		if ( single_call ) updateMyGeometry();
		return page;
	}

	void AlgoGfx::updateState( const ResultCache &res, bool animated )
	{
		// OK, das ist die harte Arbeit.  Ich erhalte einen ResultCache und soll die bestSolutions
		// daraus anzeigen.
		// Schritt #1: erstmal bei allen Chunks die Zugehörigkeit zurücksetzen
		for ( auto &i : _chunk_pos ) i.setW( -1 );
		// Schritt #2: Zugehörigkeiten aus dem Cache lesen und setzen (positionen werden später
		// berechnet, wenn das Layout klar ist...), den Pages ihren Content geben (Text/Size),
		// den entsprechenden Chunks schon ihren Versatz innerhalb der Page berechnen.
		for ( const auto &[ pid, p ] : res.asKeyValueRange() )
		{
			auto gbs  = !p.solution.isEmpty();
			auto out  = !gbs && p.selection.count() == 1 && p.selection.first() == -1;
			auto done = gbs && p.solution == p.selection;
			auto tx	  = gbs	  ? u""_s
						: out ? u"not considered anymore - paged out..."_s
							  : u"... waiting ..."_s;
			auto bts  = 0;
			if ( gbs )
			{
				auto first = true;
				auto px	   = bsf() * ( p.start_address & 0xff ) + hm;
				for ( auto cid : p.solution )
				{
					auto cb = _chunks[ cid ]->bytes();
					tx += u"%3Chunk #%1 ($%2)"_s.arg( cid ).arg( cb ).arg( first ? first = false,
																		   "... got: " : " + " );
					bts += cb;
					_chunk_pos[ cid ] = { px, float( vm + fh + vs ), 0.f, float( pid ) };
					px += bsf() * cb;
				}
			}
			_pages[ pid ]->setCurrentResult( tx, bts,
											 out	? Page::deactivated
											 : done ? Page::finished
													: Page::none );
		}
		// Schritt #3: Die Zugehörigkeiten sind klar - das Layout muss gemacht werden!
		updateMyGeometry();
		// Zuerst die Pages zurecht rücken, damit die Positionen der verteilten Chunks einfacher
		// berechnet werden können
		layoutPages( animated );
		layoutBulk( animated );
		// Schlussendlich nochmal durch die Lösungen gehen und den Chunks ihre Zielposition zuweisen
		// ...
		for ( const auto &[ pid, p ] : res.asKeyValueRange() )
		{
			auto p0 = _page_pos[ pid ];
			for ( const auto &cid : p.solution )
			{
				_chunk_pos[ cid ] += QVector4D{ p0 }; // Verschiebung in Page + Verschiebung Page
				_chunks[ cid ]->to( _chunk_pos[ cid ].toVector3D(), animated, { -256.f, 0.f },
									{ 0.f, -128.f } );
			}
		}
		// Puh, das sollte es gewesen sein ...
	}

	void AlgoGfx::updateMyGeometry()
	{
		/*  Hier wird die Gesamtgröße berechnet, die dann das Layout zurückliefert, sodass es als
		 *  einfaches QGraphicsWidget funktioniert und ich mir über die Größe keine Sorgen machen
		 * muss.
		 *
		 *  Ich möchte das Widget möglichst nah an die verfügbare Größe heran bringen.  Diese muss
		 * ich mir also zuerst besorgen.  Da die Klasse dafür zu gekapselt ist, geht das nicht so
		 * leicht. Deshalb habe ich Übergabe und einen Resize-Slot für das viewResized-Event bereit
		 * gestellt.
		 *
		 * Größenberechnung der Chunks für die verschiedenen Layout-Varianten:
		 *  ->  HeightForWidth - horizontale Ausrichtung der Chunks
		 *      -   Breite durch die Spalten vorgegeben
		 *  ->  WidthForHeight - Vertikale Ausrichtung
		 *      -   Höhe durch Anzahl der Pages / Spalten vorgegeben
		 *  ->  Ich muss mir die Positionen schlauer speichern.  Ich brauche ja nicht nur die
		 * Position, sondern auch die Rotation.  Und wenn ich schonmal dabei bin, kann ich einen
		 * Vektor auch voll machen, und die Page-ID in W schreiben.
		 *  ->  Lustig: height for width ist ja das Selbe wie width for height, wenn die Elemente eh
		 * um 90° gedreht werden. Hihi, doch nur eine Routine ;)
		 */
		// Page Basisgrößen:
		auto			pc = _pages.count();
		auto			w0 = bs() * 256 + 2. * hm;
		auto			h0 = ( fh + vm ) * 2. + vs;
		// Ich rechne gleich mehrere Größen aus.  Dazu bestimme ich delta_AR für jede Anzahl Spalten
		// in "Drüber" und "Daneben" und bestimme das Minimum. Schwupps, habe ich die nächstgelegene
		// Konfiguration gefunden ;)
		QList< QSizeF > ar;
		int				min_ar_diff_id( 0 );
		// Machen mehr als 3 Spalten Sinn?  Ich denke kaum.  Ggf. wäre eine zu große Abweichung von
		// der AR auch eine Abbruchbedingung.
		for ( auto cols = 1; cols < ( pc < 7 ? 2 : pc < 16 ? 3 : 4 ); cols++ )
		{
			auto rc = qreal( pc ) / cols;
			auto h	= h0 * rc + ( rc - 1 ) * vs;
			auto w	= w0 * cols + ( cols - 1 ) * hs;
			auto hc = freeChunkOtherSize( w );
			auto wc = freeChunkOtherSize( h );
			// Chunks oben, Pages unten:
			ar.append( { w, ( h + hc ) } );
			if ( cols > 1 )
				if ( qAbs( _sizeRatioTarget - ( w / ( h + hc ) ) )
					 < qAbs( _sizeRatioTarget
							 - ( ar[ min_ar_diff_id ].width() / ar[ min_ar_diff_id ].height() ) ) )
					min_ar_diff_id = ar.count() - 1;
			// Chunks neben Pages:
			ar.append( { ( w + wc + ( cols * hs ) ), h } );
			if ( qAbs( _sizeRatioTarget - ( ( w + wc + ( cols * hs ) ) / h ) )
				 < qAbs( _sizeRatioTarget
						 - ( ar[ min_ar_diff_id ].width() / ar[ min_ar_diff_id ].height() ) ) )
				min_ar_diff_id = ar.count() - 1;
		}
		// Ok, wir haben Aspect Ratios berechnet - schlau genug auch gleich das Minimum bestimmt ...
		// das heisst: aus dieser ID können wir jetzt das Layout und somit seine Größe ablesen.
		_sbs		= ( min_ar_diff_id & 1 );
		_cols		= ( min_ar_diff_id >> 1 ) + 1;
		_rect		= { {}, ar[ min_ar_diff_id ] };
		_pageOffset = {
			_sbs && _cols == 1 ? ar[ min_ar_diff_id ].width() - w0 : 0.,
			_sbs ? 0.
				 : ar[ min_ar_diff_id ].height()
					   - ( h0 * qreal( pc ) / _cols + ( qreal( pc ) / _cols - 1 ) * vs ) };
		_bulkOffset = { _sbs && _cols > 1 ? w0 + hs : 0., 0. };
		resize( ar[ min_ar_diff_id ] );
	}

	void AlgoGfx::viewSizeChanged( QSize s )
	{
		_sizeRatioTarget = qreal( s.width() ) / s.height();
		updateMyGeometry();
		qDebug() << "AlgoGfx::viewSizeChanged() done.";
	}

	void AlgoGfx::performLayout( bool animated )
	{
		if ( !_rect.isValid() ) updateMyGeometry();
		layoutPages( animated );
		layoutBulk( animated );
		qDebug() << "AlgoGfx::performLayout() done.";
	}

	bool AlgoGfx::sceneEvent( QEvent *e )
	{
		switch ( e->type() ) // wir machen nur unser eigenes Ding
		{
			case QEvent::LayoutRequest:
				performLayout( isVisible() );
				qDebug() << "AlgoGfx::LayoutRequest() done.";
				break;
			case QEvent::Show:
				performLayout( true );
				qDebug() << "AlgoGfx::Show() done.";
				break;
			case QEvent::Resize: emit resized(); break;
		}
		return QGraphicsWidget::sceneEvent( e );
	}

	qreal AlgoGfx::freeChunkOtherSize( qreal width )
	{
		// es ist eine Breite gegeben.  Ich iteriere mal eben die Positionen der nicht zugeordneten
		// Elemente durch ...
		auto p = QVector2D{};
		for ( auto cid = 0u; cid < _chunks.count(); cid++ )
			if ( _chunk_pos[ cid ].w() < 0.f )
			{
				auto cw = _chunks[ cid ]->bytes() * bs();
				if ( p.x() > 1 && p.x() + cw > width ) p[ 0 ] = cw + hs, p[ 1 ] += vs + fh;
				else p[ 0 ] += cw + hs;
			}
		return p.y() + vs + fh;
	}

	void AlgoGfx::layoutBulk( bool animated )
	{
		// Das Layout wurde vorher klar bestimmt.  Das heisst, ich kann daraus die erlaubte Breite
		// ablesen.
		auto width = float( _sbs ? _rect.height() : _rect.width() );
		auto p	   = QVector2D{};
		auto v	   = QVector4D{};
		for ( auto cid = 0u; cid < _chunks.count(); cid++ )
			if ( _chunk_pos[ cid ].w() < 0.f )
			{
				auto cw = _chunks[ cid ]->bytes() * bs();
				if ( p.x() > 1 && p.x() + cw > width ) p[ 0 ] = 0., p[ 1 ] += vs + fh;
				v = _sbs ? QVector4D{ p.y(), width - p.x(), -90.f, -1 } : QVector4D{ p, 0, -1 };
				if ( !qFuzzyCompare( _chunk_pos[ cid ], v + QVector4D{ _bulkOffset } ) )
				{
					_chunks[ cid ]->to( v.toVector3D() + QVector3D{ _bulkOffset }, animated );
					_chunk_pos[ cid ] = v + QVector4D{ _bulkOffset };
				}
				p[ 0 ] += cw + hs;
			}
	}

	void AlgoGfx::layoutPages( bool animated )
	{
		// ... so, das Gleiche nochmal mit den Pages ...
		auto   cm1	= qMax( 0, _cols - 1 );
		auto   w0	= bs() * 256 + 2. * hm;
		auto   h0	= ( fh + vm + vs ) * 2.;
		// Spalten-x-Position
		double co[] = { 0.,
						_sbs ? _rect.width() - ( cm1 * w0 ) - ( qMax( 0, cm1 - 1 ) * hs ) : w0 + hs,
						_sbs ? _rect.width() - w0 : 2 * ( w0 + hs ) };
		auto   pos	= QPointF{};
		int	   id	= 0;
		for ( auto i : _pages.keys() )
		{
			auto  p	 = _pages[ i ];
			auto &pp = _page_pos[ i ];
			pos.setX( co[ id % _cols ] );
			if ( !qFuzzyCompare( pp, pos + _pageOffset ) )
				p->to( pp = pos + _pageOffset, animated );
			if ( ( id++ % _cols ) == cm1 ) pos.ry() += h0;
		}
	}
} // namespace Algo
