/*
 *  This shall become a factory for Algo::Algo's Runners.
 *
 * Revisiting the current Algo design one could argue that using a communication structure and an
 * extra threading object is a waste.  The threading object, although living in the other thread,
 * can as well be used to lock and query.  It doesn't matter where it lives, it just either owns
 * the memory or not.  If it knows the address and access rights are OK, it can access it without
 * owning.
 *
 * Thus I believe a small redesign while putting it into a factory pattern seems like a good
 * solution.
 *
 * ->   the idea of letting the worker interpret data was a little off-shore.  I wanted to leave
 *      contributors as much freedom as possible, but finally I see that was over the top.
 *
 * How to change that?
 * ->   Algo will interpret the input data and simply provide the lists of chunks and pages to the
 *      CTor of derived classes.  Like "the other way around" it was before, where the runner
 *      interpreted the input and sent back the lists.
 *
 * Factory: following my own working example of the SB10X-class, there will be some simple boiler
 * plate for the factory and a big class for the pure virtual runner, derived from QObject and the
 * factory-mix-in.
 */
#pragma once

#include "AlgoData.h"
#include "ui_Algo.h"

#include <QList>
#include <QMap>
#include <QReadWriteLock>
#include <QWidget>
class QChronoTimer;

#ifdef _DESIGNER_
#	include <QtUiPlugin/QDesignerExportWidget>
#else
#	define QDESIGNER_WIDGET_EXPORT
#endif

namespace Algo
{
	using namespace Qt::StringLiterals;

#pragma region Factory-Boilerplate
	/*  Nir Friedman's "unforgettable factory" is probably the best solution
		to easing the way, new algorithms can be c0ded.

		Sorry to say, but I never got the "standard" version to run with NPOD
		CTor Arguments ... doesn't matter much, just needs an init function. */
	template < class Base >
	class Factory
	{
	  public:
		static Base *make( const QString &s )
		{
			auto x = Factory::data().find( s );
			if ( x != Factory::data().end() ) return ( *x )();
			else return nullptr;
		}
		static const QStringList keys() { return Factory::data().keys(); }
		template < class T >
		struct Registrar : Base
		{
			friend T;
			static bool registerT()
			{
				Factory::data()[ T::name_in_factory ] = []() -> Base * { return new T(); };
				qInfo() << "registered" << T::name_in_factory;
				return true;
			}
			static bool registered;

		  private:
			Registrar()
				: Base( Key{} )
			{
				( void ) registered;
			}
		};

		friend Base;

	  private:
		class Key
		{
			Key() {};
			template < class T >
			friend struct Registrar;
		};
		using FuncType = Base *( * ) ( void );
		Factory()	   = default;

		static auto &data()
		{
			static QMap< QString, FuncType > s;
			return s;
		}
	};

	template < class Base >
	template < class T >
	bool Factory< Base >::Registrar< T >::registered = Factory< Base >::Registrar< T >::registerT();
#pragma endregion

#pragma region Worker-Base
	class WorkerBase
		: public QObject
		, public Factory< WorkerBase >
	{
		Q_OBJECT

	  public:
		// This call blocks until the QReadWriteLock is free for reading.  As long as the object
		// returned is kept alive on the stack, the lock stays locked.  Upon destruction, the lock
		// is released again.
		QReadLocker		obtainReadAccess() const { return { &lock }; }
		// And this is the easy way to access one solution.  As the parent object knows all page
		// IDs, it shouldn't post a problem to iterate from there.
		const AlgoPage &solution( quint16 page_addr ) const;
		void			setInitData( ChunkList, PageList );

	  public slots:
		virtual void pause();
		virtual void iterate() = 0;

	  signals:
		void state_changed( State );
		void page_finished( quint16 pageAddress );
		void final_solution( quint64 iteration_count,
							 quint64 selection_count,
							 quint64 deselection_count,
							 qreal	 result_quality );

	  protected:
		// Finally, there needs to be a way for derived classes to initialize the internal Data, as
		// their CTors do not have access to this class' CTor because they use the Registrar for
		// derrivation.
		void sort_into_avail( int chunk_id );
		// Take Available returns true when a page got finished
		bool take_available();
		// Make Available returns true, when backtracking dropped the current page.
		bool make_available();

	  protected slots:
		// handles actual pausing
		void pre_re_iterate();

	  signals:
		void redo( bool from_inside = true );

	  protected:
		int				 a_id{ 0 }, p_id{ -1 }; // available- and page-IDs
		ChunkList		 chunks;				// in the first place, only size is needed!
		PageMap			 pages;
		QList< int >	 avail;		// chunk IDs available in "bulk"
		QList< quint16 > pageOrder, // sorted list of Page IDs in processing order,
			badPages; // pages sorted out due to obviously not being solvable with the current data
					  // set (i.e. just can't figure any way to fill the chunk with available data)
		State				   current_state;
		quint64				   cnt_sel{ 0 }, cnt_unsel{ 0 }, iteration{ 0 }, lastIt{ 0 };
		int					   cur_btsleft_thresh{ 0 };
		mutable QReadWriteLock lock;

		// Factory requirements: CTor needs a Key, and it needs to be in the Header, as short as
		// possible!
	  public:
		explicit WorkerBase( Key )
			: QObject( nullptr ) {};
		virtual ~WorkerBase() = default;
	};
	using AlgoWorker = Factory< WorkerBase >;
#pragma endregion

#pragma region AlgoFactory
	class AlgoGfx;

	class QDESIGNER_WIDGET_EXPORT Algorithm
		: public QWidget
		, public Ui::Algo
	{
		Q_OBJECT
		// A description for the algorithm in Markup
		Q_PROPERTY( QString description READ getDesc WRITE setDesc )
		Q_PROPERTY( // shall the widget provide the showDescription-Feature?
			bool showDescription MEMBER showDesc WRITE setShowDesc NOTIFY showDescriptionChanged )
		// How many priorities/penalty multiplier shall be created?
		Q_PROPERTY( int priorityCount READ getNPrio WRITE setNPrio )
		// Show the animation group box (silent,visual,animated,slider+duration display)
		Q_PROPERTY( bool showAnimationConfig MEMBER showAniCfg WRITE setShowAniCfg NOTIFY
						showAnimationCfgChanged )
		// How to report progress - silent: just numeric feedback, visual: place objects according
		// current calculation, animated: animates objects to their current positions.
		Q_PROPERTY( ProgressDisplay progressReporting READ getProgressReporting WRITE
						setProgressReporting NOTIFY progressReportingChanged )
		Q_PROPERTY( int AnimationDuration READ getAniDur WRITE setAniDur )

	  public:
		typedef enum { silent, visual, animated } ProgressDisplay;

		Algorithm( QWidget *parent = nullptr );
		~Algorithm();
		bool			eventFilter( QObject *o, QEvent *e ) override;
		QString			getDesc() const { return te_expl->toPlainText(); }
		int				getNPrio() const { return sb_prio_cnt->value(); }
		ProgressDisplay getProgressReporting() const { return prgMode; }
		int				getAniDur() const { return sl_aniDur->value(); }

	  public slots:
		void clearWorker();
		void setDesc( QString md );
		void setShowDesc( bool show );
		void setNPrio( int n );
		void setShowAniCfg( bool s );
		void setProgressReporting( ProgressDisplay x );
		void setAniDur( int v ) { sl_aniDur->setValue( v ); }

		void on_sl_aniDur_valueChanged( int );
		// Umschaltung des Visual Mode - etwas komplizierter ...
		void on_bg_visMode_idPressed( int button_id );
		void on_bg_visMode_idReleased( int button_id );
		void on_tb_expl_clicked();	   // Erläuterung anzeigen / maximieren
		void on_tb_hideExpl_clicked(); // Erläuterung verbergen
		void on_pb_generate_clicked();
		void on_pb_simulate_clicked();
		// Callbacks from the worker:
		// ->   current state containing iteration number and page worked on,
		//      as well as "paused"-state.
		void stateChange( State new_state );
		// ->   informs about finding a solution for a specific page (i.e. cache dirty signal)
		void pageFinished( quint16 page_id );
		// ->   Finalizer - returns counters and if the given solution is good (like 1.0 -> all
		// great, 0.0 -> super bad).
		void finalSolution( quint64 iteration_count,
							quint64 selection_count,
							quint64 deselection_count,
							qreal	results_good );
		void idleProcessing();

	  signals:
		void showDescriptionChanged( bool );
		void showAnimationCfgChanged( bool );
		void progressReportingChanged( ProgressDisplay );
		void stateMessageChanged( QString );
		void do_pause();
		void do_continue();

	  private:
		QPair< quint16, quint16 > read_page_info( const QString tx ) const;
		quint16					  read_maybehex( const QString tx ) const;

		ProgressDisplay			  prgMode{ animated };
		QChronoTimer			 *idleProc;
		QThread					 *thread{ nullptr };
		WorkerBase				 *worker{ nullptr };
		AlgoGfx					 *gfx{ nullptr };
		State					  lastSt;
		QSet< quint16 >			  updateRequests;
		bool					  showAniCfg{ true }, showDesc{ true };
	};
#pragma endregion
} // namespace Algo
