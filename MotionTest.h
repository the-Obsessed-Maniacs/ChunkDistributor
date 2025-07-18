#pragma once

#include "AnimatePosition.h"
#include "ui_MotionTest.h"

#include <QGraphicsWidget>

class QGraphicsTextItem;

class AP_Obj
	: public QGraphicsWidget
	, public AnimatePosition< AP_Obj >
{
	Q_OBJECT

  public:
	explicit AP_Obj( int default_ms_duration = 1000, QGraphicsItem *parentItem = nullptr );
	virtual ~AP_Obj() = default;

	void paint( QPainter *painter, const QStyleOptionGraphicsItem *option,
				QWidget *widget ) override;

  protected:
  private:
};

class FilterOrigin : public QGraphicsObject
{
  public:
	FilterOrigin( QGraphicsItem *parent = nullptr )
		: FilterOrigin( {}, parent )
	{}
	explicit FilterOrigin( QSizeF sz, QGraphicsItem *parent = nullptr );
	virtual ~FilterOrigin() = default;
	QRectF boundingRect() const override;
	void   paint( QPainter *painter, const QStyleOptionGraphicsItem *option,
				  QWidget *widget ) override;

	bool   addFilterItem( QGraphicsItem *item );

	bool   eventFilter( QObject *o, QEvent *e ) override;

  protected:
	bool sceneEventFilter( QGraphicsItem *w, QEvent *e ) override;

  private:
	QSizeF					 mySz;
	QList< QGraphicsItem * > filteredItems;
};

class MotionTest
	: public QWidget
	, private Ui::MotionTest
{
	Q_OBJECT

  public:
	MotionTest( QWidget *parent = nullptr );
	~MotionTest();

	bool eventFilter( QObject *o, QEvent *e ) override;

  public slots:
	void on_tbl_clicked();
	void on_tbr_clicked();
	void on_tbu_clicked();
	void on_tbd_clicked();

	void on_customContextMenuRequested( const QPoint &pos );

  private:
	AP_Obj		 *w{ nullptr };
	FilterOrigin *f{ nullptr };
};
