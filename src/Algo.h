#pragma once
/**************************************************************************************************
 *  Mit dieser Klasse möchte ich meinen Chunk-Verteiler-Algorithmus für den ATMDS3-Packer verallge-
 *  meinern.
 *  Das Problem tritt beim Democoden sicherlich des Öfteren auf.  Man hat spezielle Routinen, die
 *  an spezifischen Speicherpositionen stehen müssen und damit die Pages "clobbern".  Diese Seiten
 *  noch mit C0de zu füllen, ohne immer wieder in Probleme zu rennnen, ist Schwerstarbeit.
 *  Dennoch braucht der meiste Code auch viele kleine Tabellen oder Datengräber, die halt nicht
 *  gleich eine ganze Page lang sind.  Bei denen so ziemlich egal ist, wo sie liegen, Hauptsache im
 *  Speicher ;)
 *
 *  Zusammenfassend gibt es also Speicherbereiche, die "aufgefüllt" werden sollen, ggf. auch
 *  Speicherbereiche, die frei sind (zum weiteren Verteilen von Daten, nachdem die "aufzufüllenden"
 *  Bereiche schon befüllt wurden) und natürlich gibt es Datenpakete.
 *
 **************************************************************************************************
 *  Eine Umsetzung in KickAssembler würde ich mir wie folgt vorstellen:
 * ---------------------------------------------------------------------
 *  -   es gibt ein Macro bzw. eine neue Direktive ".datastore" - diese bewirkt, dass die aktuelle
 *      Speicherseite ">( * )" mit dem Versatz "<( * )" als aufzufüllende Speicherseite markiert
 *      wird.
 *  -   Zur Definition von Daten: es sollten mehrere Möglichkeiten angeboten werden.  Zum Einen
 *      soll es möglich sein, die Daten zu generieren - also eine Art "Byteliste übergeben".  Hier
 *      braucht es noch gute Ideen, wie bei Platzierung der Byteliste im Speicher Labels angelegt
 *      bzw. aktualisiert werden.
 *      Zum Anderen wäre ein Quelltext-Block gut, der so variabel im Speicher platziert werden
 *      würde.  Damit wäre auch das Label-Problem gelöst - der Block muss halt von der Engine ein-
 *      fach verarbeitet werden.
 *      =>
 *      .datachunk [name] { Code Block }
 *      .datachunk [name,data=List( Bytes ),labels=List( Strings ),labelOffsets=List( Integers )]
 **************************************************************************************************
 * Diese Klasse an sich:
 * -    soll als Basis dienen, damit man relativ einfach weitere Algorithmen umsetzen kann
 * -    Bietet die Konfigurationseinstellungen (mit ein paar Extras, die ich mir habe einfallen
 *      lassen für den Designer), Platz für eine ein- und ausblendbare Erläuterung im Markdown-
 *      Format (könnte ich gar live editierbar machen?), und den GraphicsView v, in dem Alles visu-
 *      alisiert wird.
 * ->   praktisch sollte man dann also eine neue Klasse mit dieser als UI-Basis bauen können, das
 *      muss ich dann für die Grundimplementation ausprobieren.
 * ->   Dann implementiert man einen eigenen Worker nach dem Schema des Algo-Workers.
 *
 * Kommunikation:
 * ==============
 * Der Worker erhält einen Init-Call mit den Text-Daten, sprich Chunk-Größen und Pages/Page-Ranges
 * dazu eine Liste an Prioritäten (falls durch die UI so eingestellt).  Dieser Aufruf passiert zwar
 * noch im UI-Thread, weil er "schnell" ist, dennoch ist die Rückmeldung über eine queued Connec-
 * tion ausgeführt.  So wird gewährleistet, dass die UI erst nach dem eigentlichen Starten der Re-
 * chenaufgabe aktiv wird, um den Grundzustand darzustellen.
 *
 * Nach der Initialisierung wird der Worker in den Arbeitsbereich des Worker-Threads gelegt,
 * bekanntermaßen alle Verbindungen zur Kommunikation hergestellt und schlussendlich gestartet.
 *
 * Der Worker könnte nun jedes Zwischenergebnis melden, oder einfach nur volle Seiten, oder fertig.
 * Da das mathematische zugrunde liegende Problem sehr lösungsreich, aber eine Parallelisierungs-
 * möglichkeit für mich nicht erkennbar ist, sollte er optimiert sein, sodass er absichtlich nur
 * dann in seine Eventloop "zurückschaut", wenn ein großer Abschnitt (z.B. eine Page) abgeschlossen
 * ist.  Vielleicht wäre auch die
 *************************************************************************************************/
#include "AlgoData.h"
#include "ui_Algo.h"

#include <QWidget>

#ifdef _DESIGNER_
#	include <QtUiPlugin/QDesignerExportWidget>
#else
#	define QDESIGNER_WIDGET_EXPORT
#endif
namespace Algo
{
	class AlgoRunner;
	class AlgoGfx;

	class QDESIGNER_WIDGET_EXPORT Algo
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
		typedef enum : quint64 {
			// 4 MS_Bits für den Zustand des Threads:
			// - initialized, running, pause requested/paused
			deeper_calc =
				( 1ull << 63 ), // There are no perfect solutions - 2nd Stage, of algorithm
			running_bit = ( 1ull << 62 ), // It indeed startet and a calculation is in progress
			pause_bit =
				( 1ull << 61 ), // Whenever a pause is requested, as long as the pause shall be held
			init_bit  = ( 1ull << 60 ), // Data is initialized
			// 16 Bits Page-ID
			page_bits = 44,
			page_mask = ( 0xffffull << page_bits ),
			// Masken und Helfer.
			step_mask = 0x00000fffffffffffull,
			bits_mask = 0xf000000000000000ull,
			unpause	  = pause_bit ^ bits_mask,
			dopause	  = running_bit ^ bits_mask,
			invalid	  = 0
		} State;
		static constexpr quint16 currentPage( State s ) { return ( s & page_mask ) >> page_bits; }
		static constexpr quint64 currentStep( State s ) { return ( s & step_mask ); }
		static constexpr State
		fromValues( quint64 bits, quint16 current_page, quint64 current_step )
		{
			return static_cast< State >( ( bits & bits_mask )
										 | ( static_cast< quint64 >( current_page ) << page_bits )
										 | ( current_step & step_mask ) );
		}
		using RunnerCreatorFunc = std::function< AlgoRunner *( AlgoCom & ) >;

		Algo( QWidget *parent = nullptr );
		~Algo();
		bool			eventFilter( QObject *o, QEvent *e ) override;

		QString			getDesc() const { return te_expl->toPlainText(); }
		int				getNPrio() const { return sb_prio_cnt->value(); }
		ProgressDisplay getProgressReporting() const { return prgMode; }
		int				getAniDur() const { return sl_aniDur->value(); }

		void			setDesc( QString md );
		void			setShowDesc( bool );
		void			setNPrio( int );
		void			setShowAniCfg( bool );
		void			setProgressReporting( ProgressDisplay );
		void			setAniDur( int v ) { sl_aniDur->setValue( v ); }

		// Wichtig: eine Factory für den Runner definieren (normalerweise so etwas wie
		// "[](&param){ return new MyRunner( param ); }" )
		void			setRunnerCreator( RunnerCreatorFunc _fkt ) { createRunner = _fkt; }

	  public slots:
		void on_sl_aniDur_valueChanged( int );
		// Umschaltung des Visual Mode - etwas komplizierter ...
		void on_bg_visMode_idPressed( int button_id );
		void on_bg_visMode_idReleased( int button_id );

		void on_tb_expl_clicked();	   // Erläuterung anzeigen / maximieren
		void on_tb_hideExpl_clicked(); // Erläuterung verbergen
		void on_pb_generate_clicked();
		void on_pb_simulate_clicked();
		// Callbacks from the worker:
		// ->   base data listing
		void on_init_completed( QList< quint32 >, QMap< quint16, quint32 > );
		// ->   current state containing iteration number and page worked on,
		//      as well as "paused"-state.
		void on_state_changed( State );
		// ->   informs about finding a solution for a specific page (i.e. cache dirty signal)
		void on_page_finished();
		// ->   Finalizer - returns counters and if the given solution is good (like 1.0 -> all
		// great, 0.0 -> super bad).
		void on_final_solution( quint64 iteration_count,
								quint64 selection_count,
								quint64 deselection_count,
								qreal	results_good );
		// ->   Finalizer 2 wenn klar ist, dass der Thread fertig ist - eigentlich unnötig!
		void on_runnerThread_finished();
		// Idle-Processing: soll die Qt-Message-Queue entlasten.  Erst hier neue Ergebnisse
		// visualisieren!
		void idle_processing();

	  signals:
		void showDescriptionChanged( bool );
		void showAnimationCfgChanged( bool );
		void progressReportingChanged( ProgressDisplay );
		void stateMessageChanged( QString );
		void do_pause();
		void do_continue( bool );

	  protected:
	  private:
		// Intere Zeiger / Stati:
		// ----------------------> Thread und Kommunikation:
		RunnerCreatorFunc createRunner{ nullptr };
		QThread			 *runnerThread{ nullptr };
		AlgoCom			  runnerCom;
		// ----------------------> Visualisierung:
		QChronoTimer	 *idleProc{ nullptr };

		ProgressDisplay	  prgMode{ animated };
		Algo::State		  lastSt{ invalid };
		bool			  showDesc{ true }, showAniCfg{ true }, newResults{ false };

		AlgoGfx			 *gfx{ nullptr };
	}; // namespace Algo

	__forceinline Algo::State &operator|=( Algo::State &a, const quint64 &b )
	{
		return a = static_cast< Algo::State >( a | b );
	}

	__forceinline Algo::State &operator&=( Algo::State &a, const quint64 &b )
	{
		return a = static_cast< Algo::State >( a & b );
	}

	class AlgoRunner : public QObject
	{
		Q_OBJECT

	  public:
		AlgoRunner( AlgoCom & );
		virtual ~AlgoRunner() = default;

	  public slots:
		// virtual - you may decide what to do with those strings
		virtual bool init( QString chunk_sizes, QString page_descriptions )
		{
			return init( chunk_sizes, page_descriptions, {} );
		};
		// Ready2use base implementation.
		virtual bool init( QString chunk_sizes, QString page_descriptions, QStringList priorities );
		virtual void pause();
		virtual void iterate() = 0;

	  signals:
		void state_changed( Algo::State );
		void init_data_completed( QList< quint32 >, QMap< quint16, quint32 > );
		void page_finished();
		void final_solution( quint64 iteration_count,
							 quint64 selection_count,
							 quint64 deselection_count,
							 qreal	 result_quality );

	  protected:
		void					  sort_into_avail( int chunk_id );
		// Take Available returns true when a page got finished
		bool					  take_available();
		// Make Available returns true, when backtracking dropped the current page.
		bool					  make_available();
		QPair< quint16, quint16 > read_page_info( const QString tx ) const;
		quint16					  read_maybehex( const QString tx ) const;
		int bytes( int chunk_id ) const { return chunks[ chunk_id ] & 0xffff; }
		int prio( int chunk_id ) const { return ( chunks[ chunk_id ] >> 16 ) & 0xffff; }

	  protected slots:
		// handles actual pausing
		void pre_re_iterate();

	  signals:
		void redo( bool from_inside = true );

	  protected:
		int				 a_id{ 0 }, p_id{ -1 }; // available- and page-IDs
		QList< quint32 > chunks;				// in the first place, only size is needed!
		QList< int >	 avail;					// chunk IDs available in "bulk"
		QList< quint16 > pageOrder,				// sorted list of Page IDs in processing order,
			badPages; // pages sorted out due to obviously not being solvable with the current data
					  // set (i.e. just can't figure any way to fill the chunk with available data)
					  // Communication data structure
		AlgoCom	   &com;
		Algo::State current_state;
		quint64		cnt_sel{ 0 }, cnt_unsel{ 0 }, iteration{ 0 }, lastIt{ 0 };
		int			cur_btsleft_thresh{ 0 };
	};

	class AlgoEngine : public AlgoRunner
	{
		Q_OBJECT

	  public:
		explicit AlgoEngine( AlgoCom &com )
			: AlgoRunner( com )
		{}
		virtual ~AlgoEngine() = default;

	  public slots:
		void iterate() override;

	  protected:
		void iterate_complete();
	};
} // namespace Algo

/* Gute Werte zum mehrfach testen:
189,65,194,71,62,48,170,195,19,107,135,105,164,136,151,198,158,99,162,49,124,16,136,160,48,6,147,151,180,187,71,154,25,155,196,30,39,44,23,109,70,135,139,5,45,199,42,124,180,94,87,65,163,152,160,84,44,112,86,158,73,144,56
$082d,$090e,$0a64,$0b97,$0c38,$0d40,$0e6f,$0f8a,$109e,$1192

... ~7Mio Iterationen bis Ergebnis ...
159,124,109,12,148,82,29,112,39,141,147,95,36,47,109,60,45,150,133,1,181,127,117,195,14,153,35,189,172,100,44,190,67,24
$089d,$0989,$0a9e,$0b37,$0c11,$0d96,$0e96,$0f50,$1033,$1176,$1294,$1363,$1454,$151a,$1615,$170d,$1829,$1955,$1a9f,$1b78,$1c12,$1d21,$1e3d,$1f44,$2086,$218e,$2255,$235c

~4h Rechenzeit, 511Mio Iterationen, > 11 Milliarden Sel/Unsel-Ops. - Ergebnis: 1 Chunk paged out,
alle Anderen: 0 Bytes left, ein paar Chunks übrig. Eigentlich die optimale Ausbeute!
37,35,187,127,22,32,175,94,189,133,128,182,190,39,31,62,128,57,41,128,25,85,60,151,14,69,91,141,126,42,140,91,135,109,58,163,4,32,72,5,102,160,169,54,187,166,66,154
$08a5,$09a5,$0a2e,$0b5d,$0ca5,$0d44,$0e5d,$0f60,$101e,$1171,$1252,$1325,$1482,$1526,$1655,$17a6,$1810,$19a1,$1a0c,$1b67,$1c85,$1d58,$1e1d
*/
