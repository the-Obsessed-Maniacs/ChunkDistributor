/**************************************************************************************************
 *  AnimatePosition.h - Header-only Mixin
 * ---------------------------------------
 *  Eine Klasse, die QGraphicsObject-abgelittene Klassen zu selbstanimierter Bewegung verhilft.
 *
 *  Allerdings wird die QVariantAnimation etwas versteckt, sodass es schwierig werden würde, ohne
 *  eine globale Variable "isAnimating" zu prüfen.
 *  Weiterhin gab es (logischerweise) Probleme, mit einer intern abgeleiteten QXAnimation - Signale
 *  und Slots sind auf diesem von QObject & Co. lösgelösten Level noch nicht verfügbar.
 *
 *  Konsequenz und Entscheidung: ein Namespace muss her.
 *  ->  namespace: PositionAnimation
 *      - Hermite Basis
 *      - Zähler für aktive Animationen
 *      - Funktion zum Weiterschreiben der animierten Werte an sich (connection-Ziel ohne QObject)
 *
 *  ->  class: PositionAnimation -> die Klasse, die allen QGraphicsObjects Animationshelfer
 *                                  "injeziert"
 *      - zu Verwenden: beim Ableiten eigener Widgets durch Mehrfachableitung á la:
 *          class MyClass : public QGraphicsWidget, public AnimatePosition<MyClass>
 *          -> bitte den Aufruf des CTor im CTor nicht vergessen!
 *
 *      - ob die Injektion auch via auto w = new AnimatePosition<QGraphicsWidget>() funktioniert,
 *          kann ich noch nicht absehen.
 *************************************************************************************************/
#pragma once

#include <QGraphicsObject>
#include <QMatrix4x4>
#include <QVariantAnimation>
#include <type_traits>

// Ich brauche einen Namespace, weil Signale und Slots bei CRTP nicht vorhanden sind.  Alles, was
// ich tuen kann, ist Objekte durch ihre Signale mit Lambdas verbinden.  Nun brauche ich ein
// callback, welches ich über ein Lambda aufrufe.
namespace PositionAnimation
{
	// Hermite-Basis-Matrix
	static const char*		POS = "target_position";
	static const QMatrix4x4 Mh	= QMatrix4x4{ 2.f, -2.f, 1.f, 1.f, -3.f, 3.f, -2.f, -1.f,
											  0.f, 0.f,	 1.f, 0.f, 1.f,	 0.f, 0.f,	0.f }
									 .transposed();
	extern int runningAnimations, pausedAnimations, animationDuration;
	void	   update_state( QAbstractAnimation::State ns, QAbstractAnimation::State os );
} // namespace PositionAnimation

// CRTP Mixin
template < typename Derived >
class AnimatePosition
{
  public:
	AnimatePosition()
		: anim( new QVariantAnimation( static_cast< Derived* >( this ) ) )
	{
		static_assert( std::is_base_of_v< QGraphicsObject, Derived >,
					   "AnimatePosition requires QGraphicsObject as base class" );
		auto self = static_cast< Derived* >( this );
		anim->setStartValue( 0.f );
		anim->setEndValue( 1.f );
		self->connect( anim, &QAbstractAnimation::stateChanged,
					   []( QAbstractAnimation::State ns, QAbstractAnimation::State os )
					   { PositionAnimation::update_state( ns, os ); } );
		self->connect( anim, &QVariantAnimation::valueChanged,
					   [ this ]( const QVariant& value )
					   {
						   using namespace PositionAnimation;
						   animate< Derived >( this, value.toFloat() );
					   } );
		self->setProperty( PositionAnimation::POS, QPointF{} );
	}
	void toParent( QGraphicsObject* newParent, QPointF target_pos_parentCoord, bool animated,
				   QVector2D inbound = {}, QVector2D outbound = {} )
	{
		if ( !animated ) moveToParent( newParent, target_pos_parentCoord, outbound );
		else animateToParent( newParent, target_pos_parentCoord, inbound, outbound );
	}
	void to( QPointF target_pos_parentCoord, bool animated, QVector2D inbound = {},
			 QVector2D outbound = {} )
	{
		if ( !animated ) moveTo( target_pos_parentCoord, outbound );
		else animateTo( target_pos_parentCoord, inbound, outbound );
	}
	void moveToParent( QGraphicsObject* newParent, QPointF target_pos_parentCoord,
					   QVector2D outbound = {} )
	{
		reparentSelf( newParent );
		moveTo( target_pos_parentCoord, outbound );
	}
	void moveTo( QPointF target_pos_parentCoord, QVector2D outbound = {} )
	{
		auto self = static_cast< Derived* >( this );
		self->setPos( target_pos_parentCoord );
		self->setProperty( PositionAnimation::POS, target_pos_parentCoord );
		nDir = !outbound.isNull() ? outbound : nDir;
	}
	void animateToParent( QGraphicsObject* newParent, QPointF target_pos_parentCoord,
						  QVector2D inbound = {}, QVector2D outbound = {} )
	{
		animateTo( target_pos_parentCoord, inbound, outbound, reparentSelf( newParent ) );
	}
	void animateTo( QPointF target_pos, QVector2D inbound = {}, QVector2D outbound = {},
					QPointF lastParentSP = {} )
	{
		auto obj = static_cast< Derived* >( this );
		if ( !comparesEqual( target_pos, obj->property( PositionAnimation::POS ).toPointF() ) )
		{
			auto ob	 = outbound;
			outbound = nDir;
			bool ia	 = anim->state() == QAbstractAnimation::Running;
			if ( ia ) anim->stop();
			obj->setProperty( PositionAnimation::POS, target_pos );
			// Wird überhaupt animiert?
			if ( PositionAnimation::animationDuration > 9 )
			{
				// Notwendige Geometrie-Informationen sammeln
				auto t	= float( anim->currentTime() ) / float( anim->duration() );
				auto t2 = t * t;
				auto p0 = ia ? spline * QVector4D{ t2 * t, t2, t, 1 } + QVector4D{ lastParentSP }
							 : QVector4D{ QVector2D{ obj->pos() }, 0.f, 1.f };
				auto p1 = QVector4D{ QVector2D{ target_pos }, 0.f, 1.f };
				auto m0 = ia				  ? spline * QVector4D{ t2 * 3.f, t * 2.f, 1.f, 0.f }
						  : outbound.isNull() ? p1 - p0
											  : QVector4D{ outbound };
				auto m1 = inbound.isNull() ? p1 - p0 : QVector4D{ inbound };
				// Geometriematrix erstellen
				QMatrix4x4 gm;
				gm.setColumn( 0, p0 );
				gm.setColumn( 1, p1 );
				gm.setColumn( 2, m0 );
				gm.setColumn( 3, m1 );
				spline = gm * PositionAnimation::Mh;
				nDir   = ob.isNull() ? inbound : ob;
				anim->setCurrentTime( 0 );
				anim->setDuration( PositionAnimation::animationDuration );
				anim->start();
			} else obj->setPos( target_pos );
		}
	}
	void animateStep( float progress )
	{
		auto	  t2 = progress * progress;
		QVector4D t{ t2 * progress, t2, progress, 1.f };
		QVector4D cp = spline * t;
		static_cast< Derived* >( this )->setPos( cp.toPointF() );
	}
	void changeNDir( QVector2D new_nDir ) { nDir = new_nDir; }

  protected:
	QPointF reparentSelf( QGraphicsObject* newParent )
	{
		auto self = static_cast< Derived* >( this );
		if ( newParent != self->parentItem() )
		{
			auto opsp = self->parentItem()->scenePos();
			auto prop = newParent->property( PositionAnimation::POS );
			auto npsp = prop.isValid() ? prop.toPointF() : newParent->scenePos();
			auto osp  = self->scenePos();
			self->setParentItem( newParent );
			self->setPos( osp - npsp );
			// Delta ScenePos zwischen den Parents, damit die Werte der ggf. laufenden Animation
			// umgerechnet werden können... pos->scenePos = pos+psp -> pos->newParen: pos+opsp-npsp
			return opsp - npsp;
		}
		return {};
	}

	QMatrix4x4		   spline;
	QVector2D		   nDir;
	QVariantAnimation* anim;
};

// Wie oben erläutert - das callback.  Es darf erst nach der Klasse AnimatePosition durch den
// Compiler laufen - sonst geht es nicht.
namespace PositionAnimation
{
	template < typename D >
	void animate( AnimatePosition< D >* ap, float progress )
	{
		ap->animateStep( progress );
	}
} // namespace PositionAnimation
