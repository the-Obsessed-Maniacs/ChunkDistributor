#include "MotionTest.h"

#include <QApplication>
#include <QGraphicsTextItem>
#include <QPushButton>
#include <QStyleOptionGraphicsItem>
#include <QWheelEvent>

int main( int argc, char *argv[] )
{
	QApplication app( argc, argv );
	app.setStyle( "Fusion" );
	MotionTest window;
	window.show();
	return app.exec();
}

MotionTest::MotionTest( QWidget *parent )
	: QWidget( parent )
{
	setupUi( this );
	v->setMouseTracking( true );
	v->viewport()->installEventFilter( this );
	if ( v->scene() == nullptr ) v->setScene( new QGraphicsScene( this ) );

	// Ein Widget zum Bewegen erstellen
	v->scene()->addItem( w = new AP_Obj() );
	w->setFlags( QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable
				 | QGraphicsItem::ItemSendsGeometryChanges
				 | QGraphicsItem::ItemSendsScenePositionChanges );

	v->scene()->addItem( f = new FilterOrigin( { 100., 100. } ) );
	f->addFilterItem( w );
	connect( v, &QWidget::customContextMenuRequested, this,
			 &MotionTest::on_customContextMenuRequested );
}

MotionTest::~MotionTest() {}

bool MotionTest::eventFilter( QObject *o, QEvent *e )
{
	if ( o == v->viewport() && e->type() == QEvent::Wheel )
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
			return true;
		}
	} else if ( o == v->viewport() && e->type() == QEvent::MouseButtonPress ) {
		auto qme = reinterpret_cast< QMouseEvent * >( e );
		auto sp	 = v->mapToScene( qme->pos() );
		w->animateTo( sp );
	} else if ( ( void * ) o == ( void * ) w ) {
		qDebug() << "filtered w: " << e;
	}
	return false;
}

constexpr qreal distance = 100., tangent = 100.;

void			MotionTest::on_tbl_clicked()
{
	w->animateTo( w->pos() + QPointF{ -distance, 0. }, { -tangent, 0. } );
}
void MotionTest::on_tbr_clicked()
{
	w->animateTo( w->pos() + QPointF{ distance, 0. }, { tangent, 0. } );
}
void MotionTest::on_tbu_clicked()
{
	w->animateTo( w->pos() + QPointF{ 0., -distance }, { 0., -tangent } );
}
void MotionTest::on_tbd_clicked()
{
	w->animateTo( w->pos() + QPointF{ 0., distance }, { 0., tangent } );
}
void MotionTest::on_customContextMenuRequested( const QPoint &pos )
{
	w->animateTo( { 0, 0 } );
}

FilterOrigin::FilterOrigin( QSizeF sz, QGraphicsItem *_parent )
	: QGraphicsObject( _parent )
	, mySz( sz )
{
	setFlag( QGraphicsItem::ItemHasNoContents, sz.isEmpty() );
}

QRectF FilterOrigin::boundingRect() const
{
	auto r = QRectF{ {}, mySz };
	return mySz.isEmpty() ? r
						  : r.adjusted( -r.width() / 10., -r.height() / 10., r.width() / 10.,
										r.height() / 10. );
}

QPointF operator*( const QPointF &p, const QSizeF &s )
{
	return { p.x() * s.width(), p.y() * s.height() };
}

QSizeF operator*( const QSizeF &s, const QPointF &p )
{
	return { p.x() * s.width(), p.y() * s.height() };
}

void FilterOrigin::paint( QPainter *painter, const QStyleOptionGraphicsItem *option,
						  QWidget *widget )
{
	// Paint a nice origin
	//
	auto P1	 = QPointF{ 1., 1. };
	auto psz = 0.05 * mySz;
	auto aw	 = psz.width();
	auto ah	 = psz.height();
	auto dir = QPointF{ 1, 0 };
	auto p0	 = dir * mySz;
	auto abr = QRectF{ p0 + dir.transposed() * psz, psz * -2 * ( dir + dir + P1 ) }.normalized();
	auto pp	 = QPainterPath( p0 - dir * psz );
	pp.arcTo( abr, 270., 180. );
	pp.closeSubpath();
	pp.lineTo( {} );
	// pp.clear();
	dir = dir.transposed();
	pp.moveTo( ( p0 = dir * mySz ) - dir * psz );
	abr = QRectF{ p0 + dir.transposed() * psz, psz * -2 * ( dir + dir + P1 ) }.normalized();
	pp.arcTo( abr, 180., 180. );
	pp.closeSubpath();
	pp.lineTo( {} );
	painter->drawPath( pp );
	painter->fillPath( pp, option->palette.brush( QPalette::ColorRole::Accent ) );
}

bool FilterOrigin::addFilterItem( QGraphicsItem *item )
{
	if ( item == nullptr || filteredItems.contains( item ) ) return false;

	filteredItems.append( item );
	item->installSceneEventFilter( this );
	// if ( auto io = qgraphicsitem_cast< QGraphicsObject * >( item ) ) io->installEventFilter( this
	// );
	return true;
}

bool FilterOrigin::eventFilter( QObject *o, QEvent *e )
{
	qDebug() << "object event filtered:" << e;
	return false;
}

bool FilterOrigin::sceneEventFilter( QGraphicsItem *w, QEvent *e )
{
	switch ( e->type() )
	{
		case QEvent::GraphicsSceneMove:
			qDebug() << "GraphicsSceneMove - EVENT ... die nadel im Heuhaufen?";
			break;
		case QEvent::GraphicsSceneResize:
			qDebug() << "GraphicsSceneResize - EVENT ... auch nett";
			break;
		case QEvent::GraphicsSceneWheel:
		case QEvent::GraphicsSceneContextMenu:
		case QEvent::GraphicsSceneDragEnter:
		case QEvent::GraphicsSceneDragLeave:
		case QEvent::GraphicsSceneDragMove:
		case QEvent::GraphicsSceneDrop:
		case QEvent::GraphicsSceneHelp:
		case QEvent::GraphicsSceneHoverEnter:
		case QEvent::GraphicsSceneHoverLeave:
		case QEvent::GraphicsSceneHoverMove:
		case QEvent::GraphicsSceneLeave:
		case QEvent::GraphicsSceneMouseDoubleClick:
		case QEvent::GraphicsSceneMouseMove:
		case QEvent::GraphicsSceneMousePress:
		case QEvent::GraphicsSceneMouseRelease: break;
	}
	return false;
}


static QString ObjTx( "ein OBJekkt" );

AP_Obj::AP_Obj( int default_ms_duration, QGraphicsItem *parentItem )
	: QGraphicsWidget( parentItem )
	, AnimatePosition< AP_Obj >()
{
	auto tr	 = QFontMetricsF( font() ).boundingRect( ObjTx );
	auto mrg = QMargins{ style()->pixelMetric( QStyle::PM_LayoutLeftMargin ),
						 style()->pixelMetric( QStyle::PM_LayoutTopMargin ),
						 style()->pixelMetric( QStyle::PM_LayoutRightMargin ),
						 style()->pixelMetric( QStyle::PM_LayoutBottomMargin ) }
				   .toMarginsF();
	setPreferredSize( tr.size().grownBy( mrg ) );
	// basic animation settings:
	anim->setDuration( default_ms_duration );
}


void AP_Obj::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
	painter->drawText( option->rect, Qt::AlignCenter, ObjTx );
	painter->drawRoundedRect( option->rect.adjusted( 1, 1, -1, -1 ), 4, 4 );
}
