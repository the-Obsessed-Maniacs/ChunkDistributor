#pragma once

#include "AnimatePosition.h"

#include <QGraphicsLayout>
#include <QGraphicsWidget>
#include <new>
#include <type_traits>

using namespace Qt::StringLiterals;

const qreal byteWidth = 4.0;

class QParallelAnimationGroup;
class QPropertyAnimation;
class QAbstractAnimation;
// Damit die Easing-Functionen die Super-Smooth-Step Parameter bekommen können.
template < int, typename Callable, typename Ret, typename... Args >
auto fnptr_( Callable &&c, Ret ( * )( Args... ) )
{
	static std::decay_t< Callable > storage = std::forward< Callable >( c );
	static bool						used	= false;
	if ( used )
	{
		using type = decltype( storage );
		storage.~type();
		new ( &storage ) type( std::forward< Callable >( c ) );
	}
	used = true;

	return []( Args... args ) -> Ret
	{
		auto &c = *std::launder( &storage );
		return Ret( c( std::forward< Args >( args )... ) );
	};
}
template < typename Fn, int N = 0, typename Callable >
Fn *fnptr( Callable &&c )
{
	return fnptr_< N >( std::forward< Callable >( c ), ( Fn * ) nullptr );
}

void setSST( QPropertyAnimation *ani, int i, int n, qreal t0, qreal t1 );

union Chunk
{
	quint64 v;
	struct
	{
		quint16 size, finalAddr, prio, id;
	};
	Chunk( uint s, uint p, uint i )
		: size( s )
		, prio( p )
		, finalAddr( 0 )
		, id( i )
	{}
};

class ChunkObj
	: public QGraphicsWidget
	, public AnimatePosition< ChunkObj >
{
	Q_OBJECT

  public:
	ChunkObj( QGraphicsWidget *parent, ::Chunk &&data );
	virtual ~ChunkObj() = default;

	quint16 bytes() const { return myData.size; }
	quint16 prio() const { return myData.prio; }
	quint16 id() const { return myData.id; }
	auto	txt() const { return myText; }

	QRectF	boundingRect() const override;
	void	paint( QPainter *painter, const QStyleOptionGraphicsItem *option,
				   QWidget *widget = nullptr ) override;

  public slots:

  private:
	::Chunk myData;
	QString myText, shortText;
	int		tw, sw;
};

class Chunks : public QGraphicsWidget
{
	Q_OBJECT

  public:
	Chunks( QGraphicsItem *parent );
	virtual ~Chunks() = default;

	ChunkObj *chunk( int id ) const { return lay->myChunks[ id ]; }
	quint16	  bytes( int id ) const { return chunk( id )->bytes(); }

  public slots:
	void	  addChunk( ChunkObj *co );
	void	  pack( ChunkObj *obj, bool animated = false );
	void	  pack( int chunk_id, bool animated = false ) { pack( chunk( chunk_id ), animated ); }
	ChunkObj *nimm( int chunk_id, bool animated = false );
	void	  catchUp() const;

  protected:
	// Die wahre Arbeit macht das Layout ...
	class CLA : public QGraphicsLayout
	{
	  public:
		CLA( Chunks *Parent );
		virtual ~CLA() = default;

		// Berechnet die aktuelle Position des Chunks[id], initiiert Bewegung an seinen Platz.
		// Sollten auch weitere Elemente repositioniert werden müssen, wird true zurück gegeben.
		int		sortiere_ein( int id );
		void	aktualisiere_pos( int start_s_id, bool animated = false );
		void	aktualisiere( bool animated = false ) { aktualisiere_pos( 0, animated ); }
		QPointF nxtPos( int s_id )
		{
			auto pos = targetPositions[ sortedIds[ s_id ] ];
			return nxtPos( pos, myChunks[ sortedIds[ s_id ] ]->size() );
		}
		QPointF nxtPos( QPointF &pos, QSizeF sz )
		{
			// Gleich in eine neue Zeile?
			if ( ( pos.rx() += sz.width() + hsp ) > 256 * byteWidth )
				pos.setX( 0. ), pos.ry() += sz.height() + vsp;
			return pos;
		}
		QPointF nxtLnIf( QPointF &pos, QSizeF sz )
		{
			if ( ( pos.x() + sz.width() ) > 256 * byteWidth )
				pos.setX( 0. ), pos.ry() += sz.height() + vsp;
			return pos;
		}

		// Geerbt über QGraphicsLayout
		QSizeF					sizeHint( Qt::SizeHint	which	   = Qt::PreferredSize,
										  const QSizeF &constraint = QSizeF() ) const override;
		int						count() const override;
		QGraphicsLayoutItem	   *itemAt( int i ) const override;
		void					removeAt( int index ) override;
		void					widgetEvent( QEvent *e ) override;

		Chunks				   *p;
		qreal					hsp, vsp;
		QList< int >			sortedIds;	  // die sortierten IDs, welche gerade verwaltet werden
		QRectF					childrenRect; // Platz, den die gerade verwalteten Chunks brauchen.
		QMap< int, ChunkObj * > myChunks;	  // die IDs dürfen auch gern was Spezielles sein
		QMap< int, QPointF >	targetPositions; // deshalb auch die Positionen als Map.
	};
	CLA *lay;
};

class Stage
	: public QGraphicsWidget
	, public AnimatePosition< Stage >
{
	Q_OBJECT
	const QVector2D in_tangent{ -256.f, 0.f };
	const QVector2D out_tangent{ 0.f, -256.f };

  public:
	Stage( quint16 adr, Chunks *chunkManager, quint16 _size = 0 );
	virtual ~Stage() = default;
	void		 paint( QPainter *painter, const QStyleOptionGraphicsItem *option,
						QWidget *widget = nullptr ) override;
	QRectF		 boundingRect() const override;
	QPainterPath shape() const override;
	QSizeF		 sizeHint( Qt::SizeHint which, const QSizeF &constraint = QSizeF() ) const override;

	quint16		 addr() const { return address; }
	quint16		 rest() const { return leftOver(); }
	auto		 solution() const { return sol; }

	quint16		 leftOver() const;

	auto		 badSolutions() const { return badSel; }
	void		 cache( int rest, const QList< int > &_sol ) { badSel[ rest ].append( _sol ); }

	void		 prepareReplace( const QList< int > &, bool animated = false );
	void		 replaceSolution( const QList< int > &, bool animated = false );

  public slots:
	void pack( int id, bool animated = false );
	void nimm( int id, bool animated = false );

  private:
	// Während einer Stage habe ich:
	quint16		 address, target_size; // eine Startadresse und somit auch eine ZielByteMenge.
	QList< int > sol;
	QMap< int, QList< QList< int > > > badSel; // ein Cache für bisherige Backtrack-Vorgänge
	Chunks							  *chunk_mgr;
};
