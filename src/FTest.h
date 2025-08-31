#pragma once

#include "ui_FTest.h"

#include <QGraphicsScene>
#include <QMainWindow>

class FTest
	: public QMainWindow
	, public Ui::FTest
{
	Q_OBJECT

  public:
	FTest( QWidget *parent = nullptr );
	~FTest();
	void redisplay();
	void addOffs( int offs );

  public slots:
	void on_ldy_toggled() { redisplay(); }
	void on_l10x_toggled() { redisplay(); }
	void on_xToValues_toggled() { redisplay(); }
	void on_co5_toggled() { redisplay(); }
	void on_sf_add_triggered() { addOffs( 1 ); }
	void on_sf_sub_triggered() { addOffs( -1 ); }
	void on_nn_triggered() { redisplay(); }

  protected:
	bool eventFilter( QObject *fo, QEvent *ev ) override;

  private:
	int				 SID_noteoffs{ 0 };
	// Datentabellen:
	QList< qreal >	 NoteFreqs;
	QList< quint16 > SIDnotes;
	QList< quint16 > SID_COs;
};
