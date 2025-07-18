#include "Stage.h"

#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStyleOptionGraphicsItem>

void setSST( QPropertyAnimation* ani, int i, int n, qreal t0, qreal t1 )
{
	const auto	 t0i = i * t0 / ( n - 1. );
	const auto	 t1i = t1 + ( i + 1. ) * ( 1. - t1 ) / n;
	QEasingCurve crv;
	crv.setCustomType( fnptr< qreal( qreal ) >(
		[ t0i, t1i ]( double progress ) -> double
		{
			/*Super-Smoothstep für n parallel bewegte Dinge*/
			auto t = qMax( 0., qMin( 1., ( progress - t0i ) / ( t1i - t0i ) ) );
			t	   = t * t * ( 3 - 2 * t );
			// qDebug() << "SST:" << t0i << t1i << progress << t;
			return t;
		} ) );
	ani->setEasingCurve( crv );
}

ChunkObj::ChunkObj( QGraphicsWidget* parent, Chunk&& data )
	: QGraphicsWidget( parent )
	, myData( data )
	, myText( u"Chunk #%1 - %2 Bytes"_s.arg( data.id ).arg( data.size ) )
	, shortText( u"#%1 - %2"_s.arg( myData.id ).arg( myData.size ) )
{
	auto fm = QFontMetrics( font() );
	tw		= fm.horizontalAdvance( myText );
	sw		= fm.horizontalAdvance( shortText );
	resize( boundingRect().size() );
	setToolTip( myText );
}

QRectF ChunkObj::boundingRect() const
{
	return { { 0, 0 }, QSizeF{ myData.size * byteWidth, QFontMetricsF( font() ).height() } };
}

void ChunkObj::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
	QPainterPath pp;
	pp.addRoundedRect( option->rect, 5, 5 );
	painter->strokePath( pp, painter->pen() );
	painter->fillPath( pp, QColor::fromHslF( 0.3333f * ( 2 - myData.prio ), .7f, .72f ) );

	auto tx = myText;
	if ( tw > option->rect.width() )
	{
		tx = shortText;
		if ( sw > option->rect.width() )
			tx = option->fontMetrics.elidedText( tx, Qt::ElideMiddle, option->rect.width(),
												 Qt::AlignCenter | Qt::AlignJustify );
	}
	painter->setPen( QColor::fromHslF( 1.0f - ( 0.3f * myData.prio ), .7f, .72f ) );
	painter->drawText( option->rect, Qt::AlignCenter | Qt::AlignJustify, tx );
}


Stage::Stage( quint16 adr, Chunks* chunkManager, quint16 _size )
	: QGraphicsWidget()
	, AnimatePosition< Stage >()
	, address( adr )
	, target_size( _size == 0 ? ( adr & 0xff00 ) + 0x100 - adr : _size )
	, chunk_mgr( chunkManager )
{ // Stage erstellt ihre Chunks und sortiert sie nach Größe ...
	auto dx = style()->pixelMetric( QStyle::PM_LayoutLeftMargin );
	auto dy = style()->pixelMetric( QStyle::PM_LayoutTopMargin );
	auto fs = QFontMetricsF( font() ).height();
	// Die Content Margins muss ich noch genauer festlegen:
	setContentsMargins( dx, 2 * dy + fs, dx, dy );
}

QRectF Stage::boundingRect() const
{ // Die Stage möchte immer gleich groß sein.  Nämlich damit sie Platz für 256 Byte Blöcke hat.
	// Dafür wurde "byteWidth" festgelegt als Pixelmaß pro Byte.
	auto dx = style()->pixelMetric( QStyle::PM_LayoutLeftMargin );
	auto dy = style()->pixelMetric( QStyle::PM_LayoutTopMargin );
	auto fs = QFontMetricsF( font() ).height();
	// Breite: 256bytesWidth + 2 Seiten Margin
	// Höhe: Titelzeile + 2x Margin + weiteres Margin für unten + 2x Fonthöhe
	return { {}, QSizeF{ byteWidth * 256. + 2. * dx, ( dy + fs ) * 2 + dy } };
}

QPainterPath Stage::shape() const
{
	QPainterPath pp;
	pp.addRoundedRect( boundingRect(), 2 * byteWidth, 2 * byteWidth );
	return pp;
}

QSizeF Stage::sizeHint( Qt::SizeHint which, const QSizeF& constraint ) const
{
	return boundingRect().size();
}

quint16 Stage::leftOver() const
{
	auto res = target_size;
	for ( auto i : sol ) res -= chunk_mgr->bytes( i );
	return res;
}


void Stage::prepareReplace( const QList< int >& ns, bool animated )
{ // What I wanted to do:
  // - Remove the Chunks not part of the solution anymore
	auto it = sol.begin();
	while ( it != sol.end() )
		if ( ns.indexOf( *it ) == -1 ) // Not part of new solution?
		{
			auto oc = chunk_mgr->chunk( *it ); // old Object to drop
			chunk_mgr->pack( oc, animated );   // give it to the manager
			it = sol.erase( it );
		} else ++it;
}

void Stage::replaceSolution( const QList< int >& ns, bool animated )
{
	// Neue Lösung ... dazu werden die Chunks los gelassen, die nicht mehr benötigt werden
	prepareReplace( ns, animated );
	// Dann die noch in der Lösung verbliebenen verschoben und die Neuen hinzugefügt.
	auto   pos = QPointF{};
	qreal &px = pos.rx(), &py = pos.ry();
	getContentsMargins( &px, &py, nullptr, nullptr );
	px += ( address & 0xff ) * byteWidth;
	// Zuerst werden alle Lösungsteile annektiert und an ihre Positionen verschoben
	for ( int nid = 0; nid < ns.count(); nid++ )
	{
		auto cid = ns[ nid ];
		auto sid = sol.indexOf( cid );
		auto obj = chunk_mgr->chunk( cid );
		if ( sid == -1 ) // Chunk ist nicht in der aktuellen solution?
		{
			chunk_mgr->nimm( cid, animated );
			if ( animated ) obj->animateToParent( this, pos, in_tangent, out_tangent );
			else obj->moveToParent( this, pos, out_tangent );
		} else
		{ // Chunk ist schon Child - nur bewegen, wenn nötig!
		  // (könnte schon in Bewegung sein, deshalb besser das Ziel vergleichen)
			auto cp = obj->property( PositionAnimation::POS ).toPointF();
			if ( cp != pos || obj->pos() != pos )
				if ( animated )
				{
					auto d = QVector2D{ pos - cp };
					obj->changeNDir( -d / byteWidth );
					obj->animateTo( pos, d / byteWidth, out_tangent );
				} else obj->moveTo( pos, out_tangent );
		}
		// nötige Bewegung durchgeführt.  Px aktualisieren!
		px += obj->size().width();
	}
	sol = ns; // neue Lösung festhalten
	if ( animated ) update();
}

void Stage::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
	qreal l, t, b;
	getContentsMargins( &l, &t, nullptr, &b );
	auto r	= option->rect.toRectF();
	auto cr = r.adjusted( l, t, -l, -b );
	painter->drawRoundedRect( r, 2 * byteWidth, 2 * byteWidth );
	painter->drawRoundedRect( cr.adjusted( -2, -2, 2, 2 ), byteWidth + 1, byteWidth + 1 );
	if ( address & 0xff )
	{
		auto sr = cr;
		sr.setWidth( byteWidth * ( address & 0xff ) );
		QPainterPath pp;
		pp.addRoundedRect( sr, byteWidth, byteWidth );
		painter->fillPath( pp, painter->pen().brush() );
	}
	if ( ( ( address + target_size ) & 0xff00 ) < ( ( address + 256 ) & 0xff00 ) )
	{
		auto sr = cr;
		sr.setLeft( sr.left() + ( ( address + target_size ) & 0xff ) * byteWidth );
		QPainterPath pp;
		pp.addRoundedRect( sr, byteWidth, byteWidth );
		painter->fillPath( pp, painter->pen().brush() );
	}
	auto txt = u"%3 - start: $%1, ($%2=>%4)"_s.arg( address, 4, 16, QChar{ '0' } )
				   .arg( target_size, 2, 16, QChar{ '0' } )
				   .arg( objectName() )
				   .arg( target_size );
	auto tr = r.adjusted( l, b, -l, -b ), ur = tr;
	bool pa{ option->fontMetrics.horizontalAdvance( txt ) < tr.width() - 20 };
	if ( pa )
	{
		painter->drawText( tr, Qt::AlignTop | Qt::AlignLeft, txt, &ur );
		tr.setLeft( ur.right() + l );
	}
	bool first = true;
	auto bl	   = static_cast< int >( target_size );
	txt.clear();
	for ( auto i : sol )
	{
		bl -= chunk_mgr->bytes( i );
		txt += u"%3Chunk #%1 ($%2)"_s.arg( i )
				   .arg( chunk_mgr->bytes( i ) )
				   .arg( first ? first = false, " ... got: " : " + " );
	}
	painter->drawText( tr, Qt::AlignTop | Qt::AlignRight, u"left: %1"_s.arg( bl, 3 ), &ur );
	if ( pa )
	{
		tr.setRight( ur.left() - l );
		if ( option->fontMetrics.horizontalAdvance( txt ) >= tr.width() - 2 * byteWidth )
			txt =
				option->fontMetrics.elidedText( txt, Qt::ElideMiddle, tr.width() - 2 * byteWidth );
		// Farben: da könnte ich mir einen static const Gradient machen ...
		// - Grundgröße: bl (bytes left)
		// Werte: -1     0       8        16      ...256
		//      tiefrot  grün    türkis   gelb    immer röter werdend
		const auto drtl = ( 1.f / 3.f ), gut = 8.f, hmm = 16.f;
		QColor	   col;
		if ( bl < 0 ) col = Qt::red;
		else if ( bl <= gut )
			col = QColor::fromHslF(
				qMax( qMin( drtl + ( ( 0.5f - drtl ) / gut * bl ), 0.999999f ), 0.f ),
				qMax( qMin( .8f - ( 0.08f / gut ) * bl, 0.999999f ), 0.f ), .70f );
		else if ( bl <= hmm )
			col = QColor::fromRgb(
				qRound( 123 + ( 234 - 123 ) * qMax( qMin( ( bl - hmm ) / hmm, 1.f ), 0.f ) ),
				234,
				qRound( 228 - ( 228 - 123 ) * qMax( qMin( ( bl - hmm ) / hmm, 1.f ), 0.f ) ) );
		else
			col = QColor::fromHslF( .5f * drtl * ( 1.f - qMin( bl - hmm, hmm ) / hmm ),
									( 0.999999f - ( ( bl - hmm ) / ( target_size - hmm ) ) ) * .27f
										+ .72f,
									.70f );
		painter->setPen( col );
		painter->drawText( tr, Qt::AlignTop | Qt::AlignHCenter, txt );
	}
}

void Stage::nimm( int id, bool animated )
{
	// Die Position bestimmen: Margins laden (Startposition), Bytes zählen und verschieben...
	QPointF p;
	getContentsMargins( &p.rx(), &p.ry(), nullptr, nullptr );
	int bts = ( address & 0xff );
	for ( auto sid : sol ) bts += chunk_mgr->bytes( sid );
	p.rx() += byteWidth * bts;
	// Den Chunk tatsächlich "holen"
	sol.append( id );
	chunk_mgr->nimm( id, animated )->toParent( this, p, animated, in_tangent, out_tangent );
}

void Stage::pack( int id, bool animated )
{
	auto sid = sol.indexOf( id );
	if ( sid != -1 )
	{
		chunk_mgr->pack( sol.takeAt( sid ), animated );
		if ( animated ) update();
	}
}


Chunks::Chunks( QGraphicsItem* parent )
	: QGraphicsWidget( parent )
	, lay( new CLA( this ) )
{
	lay->vsp = ( qreal ) style()->pixelMetric( QStyle::PM_LayoutVerticalSpacing );
	lay->hsp = ( qreal ) style()->pixelMetric( QStyle::PM_LayoutHorizontalSpacing );
}

// einen komplett neu erstellten Chunk hinzufügen
void Chunks::addChunk( ChunkObj* co )
{
	// Hinzufügen und an seine Position animieren
	lay->myChunks.insert( co->id(), co );
	lay->targetPositions.insert( co->id(), mapFromScene( co->scenePos() ) );
	pack( co, true );
}

// Einen zuvor entnommenen Chunk zurück packen (Animation optional!)
void Chunks::pack( ChunkObj* obj, bool animated )
{
	if ( obj )
	{
		auto id	 = obj->id();
		auto i	 = lay->sortiere_ein( id );
		auto pos = ( i > 0 ) ? lay->nxtPos( i - 1 ) : QPointF{};
		lay->nxtLnIf( pos, obj->size() );
		obj->toParent( this, pos, animated );
		lay->aktualisiere_pos( i, animated );
	}
}

// Einen existierenden Chunk aus dem Layout holen (Animation der Repositionierung optional!)
ChunkObj* Chunks::nimm( int chunk_id, bool animated )
{
	// reparenting must be done by the caller!
	int s_id = lay->sortedIds.indexOf( chunk_id );
	if ( s_id != -1 )
	{ // Safeguard ...
		lay->sortedIds.remove( s_id );
		lay->aktualisiere_pos( ( s_id > 0 ? s_id - 1 : 0 ), animated );
	}
	return lay->myChunks[ chunk_id ];
}

void Chunks::catchUp() const
{
	for ( auto i : lay->myChunks ) i->setPos( i->property( PositionAnimation::POS ).toPointF() );
}

Chunks::CLA::CLA( Chunks* Parent )
	: QGraphicsLayout( Parent )
	, p( Parent )
{
	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding,
				   QSizePolicy::ControlType::GroupBox );
}

int Chunks::CLA::sortiere_ein( int id )
{
	int	 i	= 0;
	auto co = myChunks[ id ];
	while ( i < sortedIds.count() )
	{
		if ( co->bytes() < myChunks[ sortedIds[ i ] ]->bytes() ) ++i;
		else if ( ( co->bytes() == myChunks[ sortedIds[ i ] ]->bytes() )
				  && ( co->prio() < myChunks[ sortedIds[ i ] ]->prio()
					   || co->id() > myChunks[ sortedIds[ i ] ]->id() ) )
			++i;
		else break;
	}
	sortedIds.insert( i, id );
	return i;
}

void Chunks::CLA::aktualisiere_pos( int start_s_id, bool animated )
{
	qreal maxx = 0;
	auto  i	   = start_s_id;
	auto  pos  = ( i > 0 ) ? nxtPos( i - 1 ) : QPointF{};
	while ( i < sortedIds.count() )
	{
		auto co = myChunks[ sortedIds[ i ] ];
		nxtLnIf( pos, co->size() );
		if ( !comparesEqual( pos, targetPositions[ sortedIds[ i ] ] ) )
		{
			if ( animated ) co->animateTo( targetPositions[ sortedIds[ i ] ] = pos );
			else co->setPos( targetPositions[ sortedIds[ i ] ] = pos );
			maxx = qMax( maxx, pos.x() + co->size().width() );
		}
		nxtPos( pos, co->size() );
		++i;
	}
	if ( i > 0 ) // ohne chunks macht das darunter keinen Sinn.
	{
		// Der untere Wert ist sicher
		childrenRect.setBottom( pos.y() + myChunks[ sortedIds.last() ]->size().height() );
		// Beim anderen ist nur sicher, wenn von vorn begonnen wurde, oder der Wert größer ist.
		childrenRect.setRight( start_s_id == 0 ? maxx : qMax( childrenRect.right(), maxx ) );
		updateGeometry();
	}
}

int Chunks::CLA::count() const
{
	return sortedIds.size();
}

QGraphicsLayoutItem* Chunks::CLA::itemAt( int i ) const
{
	return myChunks.value( sortedIds[ i ] );
}

// Interpretiert als komplettes Löschen eines Elementes.  Diese Funktion wird eh nicht genutzt.
void Chunks::CLA::removeAt( int index )
{
	int id = sortedIds[ index ];
	sortedIds.remove( index );
	myChunks.remove( id );
	targetPositions.remove( id );
	aktualisiere_pos( 0 );
}

// Notifier - Geometry was set
// void Chunks::CLA::setGeometry( const QRectF& rect )
//{
//	// never actually seen this output, yet ...
//	qDebug() << "CLA::setGeometry(" << rect << ")";
//}

// Called to ask for Sizes ...
QSizeF Chunks::CLA::sizeHint( Qt::SizeHint which, const QSizeF& constraint ) const
{
	QSizeF	  am  = { 256 * byteWidth, myChunks.isEmpty() ? QFontMetricsF( p->font() ).height()
														  : myChunks.first()->size().height() };
	QMarginsF lam = {
		( qreal ) p->style()->pixelMetric( QStyle::PM_LayoutLeftMargin ),
		( qreal ) p->style()->pixelMetric( QStyle::PM_LayoutTopMargin ),
		( qreal ) p->style()->pixelMetric( QStyle::PM_LayoutRightMargin ),
		( qreal ) p->style()->pixelMetric( QStyle::PM_LayoutBottomMargin ),
	};
	switch ( which )
	{
		case Qt::MinimumSize:
			if ( childrenRect.isValid() ) am = childrenRect.marginsAdded( lam ).size();
			else am = am.grownBy( lam );
			break;
		case Qt::PreferredSize:
			if ( childrenRect.isValid() ) am = childrenRect.marginsAdded( lam ).size();
			else am = am.operator+=( { 4. * hsp, 4. * ( vsp + am.height() ) } ).grownBy( lam );
			break;
		case Qt::MaximumSize:
			if ( childrenRect.isValid() ) am = childrenRect.marginsAdded( lam ).size();
			else am = { QWIDGETSIZE_MAX, QWIDGETSIZE_MAX };
			break;
	}
	return am;
}

void Chunks::CLA::widgetEvent( QEvent* e )
{
	switch ( e->type() )
	{
		// Layouten tue ich die ganze Zeit - mit jedem Element wird das Layout aktualisiert.
		case QEvent::LayoutRequest: aktualisiere_pos( 0, p->isVisible() ); break;
		case QEvent::Show:
			aktualisiere_pos( 0, true );
			break;
			// polish event bedeutet: jetzt mal layouten, was da ist.
	}
}
