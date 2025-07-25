/**************************************************************************************************
 * AlgoGfx: QGraphicsObjects/Widgets zur Visualisierung des Algorithmus.
 * =====================================================================
 * - Chunk: stellt ein Datenpaket dar
 *          ->  hat nur eine Größe in Bytes und eine ID, ggf. noch eine Prio bzw. Farbe (wird sor-
 *              tiert als "kleinste zuerst")
 * - Page:  stellt einen partiell gefüllten, aufzufüllenden Speicherbereich (hier i.d.R 256 Bytes)
 *          dar.
 *          -> Start: ab welcher Adresse Platz ist
 *          -> Ende: ab wo wieder andere Daten stehen
 * - Manager:
 *  Die beiden anderen Objekttypen brauchen wirklich keine großartige Funktionalität.  Es reicht,
 *  wenn es QGraphicsObjects sind, die mit der PositionAnimation gespickt sind.
 *
 *  Der Manager hingegen sollte ein QGraphicsWidget sein.  Er braucht auch Layout-Funktionalität,
 *  wobei das ohne Layout sowieso besser geht.  Vielleicht sollte ich Height for Width implementie-
 *  ren, um eine Initialgröße zu bekommen, die die vorhandenen Datenelemente übersichtlicher posi-
 *  tionierbar macht.
 *
 * Das bedeutet: ich könnte unterschiedliche Layouts implementieren mit dem Ziel, irgendwie in die
 * Nähe einer 16:9-Ratio zu kommen.
 *
 * Grundgrößen  Zeile:          QFontMetrics( font() ).height()
 *              Spacing:        style()->pixelMetric( PM_Layout(Vertical|Horizontal)Spacing )
 *              Chunk-Size:     Bytes * Zeile
 *              Page-Size:      (256 Bytes + 2 Spacings) * (2 Zeilen + 3 Spacings)
 *
 * Also Pi mal Auge kann ich mit Page-Size - Blöcken hantieren ... die sind übersichtlich. Zeilen
 * Höhe war beim Debuggen glaub ich 16, Spacing 9 - das passt in den Kram ;)
 * Ergo kann ich bei sehr wenigen Pages an sich einzeilig noch auf ein 16:9-Format (oder eines ähn-
 * lich dem aktuellen Viewport!) kommen.  Ansonsten kann ich vielleicht eine Formel aufstellen, ab
 * bis wie viele Zeilen ich in wie vielen Spalten agieren kann.
 * Es macht aber auch schon ab 8 Pages ggf. Sinn, die Chunks vertikal links daneben anzuordnen -
 * dann müsste ich der Posani allerdings noch eine Rotation mitgeben.  Kein Ding - ist ja ein
 * Spline im 3D-Raum - kann die Rotation also als Z eingefügt werden.  Sie braucht auch keine eige-
 * ne Tangente/Anstieg - der Anstieg für den Spline ist ja schon geregelt. ;)
 *
 * Hier noch eine Nachrecherche: das mit dem Layout klappt iwie nicht so recht - auf einmal sind
 * die Positionstabellen im Eimer und ich kapiere nicht, warum.  Das muss mit QGraphicsWidget ohne
 * extra Layout gehen:
 * -
 *************************************************************************************************/
#pragma once

#include "AlgoData.h"

#include <QGraphicsLayout>
#include <QGraphicsWidget>
#include <QMatrix4x4>
#include <QVariantAnimation>
#include <type_traits>

#pragma region CRTP-mixin_PositionAni_Mated
namespace PositionAni
{
	// Hermite-Basis-Matrix
	static const char*		POS = "target_position";
	static const char*		ROT = "target_rotation";
	static const QMatrix4x4 Mh	= QMatrix4x4{ 2.f, -2.f, 1.f, 1.f, -3.f, 3.f, -2.f, -1.f,
											  0.f, 0.f,	 1.f, 0.f, 1.f,	 0.f, 0.f,	0.f }
									 .transposed();
	extern int running, paused, duration;
	void	   update_state( QAbstractAnimation::State ns, QAbstractAnimation::State os );
	template < typename Derived >
	class Mated
	{
	  public:
		Mated()
			: anim( new QVariantAnimation( static_cast< Derived* >( this ) ) )
		{
			static_assert( std::is_base_of_v< QGraphicsObject, Derived >,
						   "AnimatePosition requires QGraphicsObject as base class" );
			auto self = static_cast< Derived* >( this );
			anim->setStartValue( 0.f );
			anim->setEndValue( 1.f );
			self->connect( anim, &QAbstractAnimation::stateChanged,
						   []( QAbstractAnimation::State ns, QAbstractAnimation::State os )
						   { PositionAni::update_state( ns, os ); } );
			self->connect( anim, &QVariantAnimation::valueChanged, [ this ]( const QVariant& value )
						   { animate< Derived >( this, value.toFloat() ); } );
			self->setProperty( POS, QPointF{} );
			self->setProperty( ROT, 0.f );
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
			else animateTo( QVector3D{ target_pos_parentCoord }, inbound, outbound );
		}
		void to( QVector3D target_pos_rot_parentCoord, bool animated, QVector2D inbound = {},
				 QVector2D outbound = {} )
		{
			if ( !animated ) moveTo( target_pos_rot_parentCoord, outbound );
			else animateTo( target_pos_rot_parentCoord, inbound, outbound );
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
			self->setProperty( POS, target_pos_parentCoord );
			nDir = !outbound.isNull() ? outbound : nDir;
		}
		void moveTo( QVector3D target_pos_rot_parentCoord, QVector2D outbound = {} )
		{
			auto self = static_cast< Derived* >( this );
			auto p	  = target_pos_rot_parentCoord.toPointF();
			self->setPos( p ), self->setProperty( POS, p );
			auto r = target_pos_rot_parentCoord.z();
			self->setRotation( r ), self->setProperty( ROT, r );
			nDir = !outbound.isNull() ? outbound : nDir;
		}
		void rotTo( float phi )
		{
			auto self = static_cast< Derived* >( this );
			self->setRotation( phi );
			self->setProperty( ROT, phi );
		}
		void animateToParent( QGraphicsObject* newParent, QPointF target_pos_parentCoord,
							  QVector2D inbound = {}, QVector2D outbound = {} )
		{
			animateTo( { target_pos_parentCoord }, inbound, outbound, reparentSelf( newParent ) );
		}
		void animateTo( QVector3D target_pos_rot, QVector2D inbound = {}, QVector2D outbound = {},
						QPointF lastParentSP = {} )
		{
			auto obj = static_cast< Derived* >( this );
			if ( !qFuzzyCompare( target_pos_rot.toPointF(), obj->property( POS ).toPointF() )
				 || !qFuzzyCompare( target_pos_rot.z(), obj->property( ROT ).toFloat() ) )
			{
				auto ob	 = outbound;
				outbound = nDir;
				bool ia	 = anim->state() == QAbstractAnimation::Running;
				if ( ia ) anim->stop();
				obj->setProperty( POS, target_pos_rot.toPointF() );
				obj->setProperty( ROT, target_pos_rot.z() );
				// Wird überhaupt animiert?
				if ( duration > 9 )
				{
					// Notwendige Geometrie-Informationen sammeln
					auto t	= float( anim->currentTime() ) / float( anim->duration() );
					auto t2 = t * t;
					auto p0 =
						ia ? spline * QVector4D{ t2 * t, t2, t, 1 } + QVector4D{ lastParentSP }
						   : QVector4D{ QVector2D{ obj->pos() }, float( obj->rotation() ), 1.f };
					auto	   p1 = QVector4D{ target_pos_rot, 1.f };
					auto	   m0 = ia ? spline * QVector4D{ t2 * 3.f, t * 2.f, 1.f, 0.f }
									: outbound.isNull() ? p1 - p0
														: QVector4D{ outbound };
					auto	   m1 = inbound.isNull() ? p1 - p0 : QVector4D{ inbound };
					// Geometriematrix erstellen
					QMatrix4x4 gm;
					gm.setColumn( 0, p0 );
					gm.setColumn( 1, p1 );
					gm.setColumn( 2, m0 );
					gm.setColumn( 3, m1 );
					spline = gm * Mh;
					nDir   = ob.isNull() ? inbound : ob;
					anim->setCurrentTime( 0 );
					anim->setDuration( duration );
					anim->start();
				} else
					obj->setPos( target_pos_rot.toPointF() ),
						obj->setRotation( target_pos_rot.z() );
			}
		}
		void animateStep( float progress )
		{
			auto	  t2 = progress * progress;
			QVector4D t{ t2 * progress, t2, progress, 1.f };
			QVector4D cp = spline * t;
			static_cast< Derived* >( this )->setPos( cp.toPointF() );
			static_cast< Derived* >( this )->setRotation( cp.z() );
		}
		void changeNDir( QVector2D new_nDir ) { nDir = new_nDir; }

	  protected:
		QPointF reparentSelf( QGraphicsObject* newParent )
		{
			auto self = static_cast< Derived* >( this );
			if ( newParent != self->parentItem() )
			{
				auto opsp = self->parentItem()->scenePos();
				auto prop = newParent->property( POS );
				auto npsp = prop.isValid() ? prop.toPointF() : newParent->scenePos();
				auto osp  = self->scenePos();
				self->setParentItem( newParent );
				self->setPos( osp - npsp );
				// Delta ScenePos zwischen den Parents, damit die Werte der ggf. laufenden Animation
				// umgerechnet werden können... pos->scenePos = pos+psp -> pos->newParen:
				// pos+opsp-npsp
				return opsp - npsp;
			}
			return {};
		}

		QMatrix4x4		   spline;
		QVector2D		   nDir;
		QVariantAnimation* anim;
	};

	template < typename D >
	void animate( Mated< D >* ap, float progress )
	{
		ap->animateStep( progress );
	}
} // namespace PositionAni
#pragma endregion

using namespace Qt::StringLiterals;
namespace Algo
{
	constexpr auto bytePixels  = 4;
	constexpr auto bytePixelsF = double( 4 );
	struct AlgoPage;

	class Chunk
		: public QGraphicsObject
		, public PositionAni::Mated< Chunk >
	{
		Q_OBJECT

	  public:
		Chunk( QGraphicsWidget* parent, uint bytes, uint id, int prio );
		virtual ~Chunk() = default;

		uint   bytes() const { return _bytes; }
		uint   id() const { return _id; }
		int	   prio() const { return _prio; }
		auto   txt() const { return _myText; }

		QRectF boundingRect() const override;
		void   paint( QPainter* painter, const QStyleOptionGraphicsItem* option,
					  QWidget* widget = nullptr ) override;

	  protected:
		uint	_bytes, _id;
		int		_prio, _w;
		QString _myText;
	};

	class Page
		: public QGraphicsObject
		, public PositionAni::Mated< Page >
	{
		Q_OBJECT
	  public:
		using Status = enum { none, finished, deactivated };
		Page( QGraphicsWidget* parent, uint start_address, uint end_address );
		virtual ~Page() = default;

		void setCurrentResult( QString content_string, int content_size,
							   Status changed_state = static_cast< Status >( -1 ) );
		auto currentContent() const { return _myContent; }
		auto currentContentSize() const { return _end_address - _start_address - _leftNow; }
		void setStatus( Status s )
		{
			setCurrentResult( currentContent(), currentContentSize(), s );
		}

		QRectF boundingRect() const override;
		void   paint( QPainter* painter, const QStyleOptionGraphicsItem* option,
					  QWidget* widget = nullptr ) override;

	  protected:
		uint	_start_address, _end_address;
		int		_leftNow;
		Status	_myState;
		QColor	_contCol;
		QString _myText, _myLeft, _myContent;
	};

	class AlgoGfx : public QGraphicsWidget
	{
		Q_OBJECT
	  public:
		static int hm, vm, hs, vs, fh;

		explicit AlgoGfx( const QSize& currentViewSize, const QList< quint32 >& chunks,
						  const QMap< quint16, quint32 >& pages );
		explicit AlgoGfx( const QSize& currentViewSize, const QList< quint32 >& chunks,
						  const QList< quint64 >& pages );
		virtual ~AlgoGfx() = default;

		Page* addNewPage( uint address, uint enda_size, bool single_call = true );
		void  updateState( const ResultCache& res, bool animated = false );
		void  changePage( const AlgoPage& pg );
		void  commitPages( bool animated = false );

		void  updateMyGeometry();

	  public slots:
		void viewSizeChanged( QSize );
		void performLayout( bool animated = false );

	  signals:
		void resized();

	  protected:
		bool				sceneEvent( QEvent* event ) override;

		void				initBuildChunks( const QList< quint32 >& chunks );
		qreal				freeChunkOtherSize( qreal width );
		void				layoutBulk( bool animated = false );
		void				layoutPages( bool animated = false );

		// Chunks und Pages - hier werden die grafischen Elemente zwischengespeichert
		QList< Chunk* >		_chunks; // IDs anhand Listenposition -> übereinstimmend mit init data
		QMap< uint, Page* > _pages;	 // IDs anhand (start_address & ~0xff) - deshalb lieber 'ne Map
									// aktuelle Positionen und Rotationen (bei den Chunks - will mir
		QList< QVector4D >	_chunk_pos; // diese Möglichkeit offen lassen, dass die Chunks im
		QMap< uint, QPointF >
					_page_pos; // "Vorrat" vielleicht senkrecht aufgereiht werden könnten ...)
		QSet< int > _moved_chunks;

		// Layout-Hilfsmittel
		QRectF		_rect; // Aktuell berechnetes Rect
		QPointF		_pageOffset;
		QPointF		_bulkOffset;
		qreal		_sizeRatioTarget; // Ziel-Aspect-Ratio (nämlich die Aktuelle des Views)
		int			_cols;
		bool		_sbs;
	};

	class AlgoLa : public QGraphicsLayout
	{
	  public:
		explicit AlgoLa( AlgoGfx* parent );
		virtual ~AlgoLa() = default;
		QSizeF sizeHint( Qt::SizeHint which, const QSizeF& constraint = QSizeF() ) const override;
		int	   count() const override { return 0; }
		QGraphicsLayoutItem* itemAt( int id ) const override { return nullptr; }
		void				 removeAt( int id ) override {}
	};

#pragma region Spacings_und_Margins
	__forceinline qreal bs()
	{
		return bytePixelsF;
	}
	__forceinline float bsf()
	{
		return static_cast< float >( bytePixelsF );
	}
	__forceinline qreal fh()
	{
		return AlgoGfx::fh;
	}
	__forceinline qreal hs()
	{
		return AlgoGfx::hs;
	}
	__forceinline qreal vs()
	{
		return AlgoGfx::vs;
	}
	__forceinline qreal hm()
	{
		return AlgoGfx::hm;
	}
	__forceinline qreal vm()
	{
		return AlgoGfx::vm;
	}
#pragma endregion
} // namespace Algo
