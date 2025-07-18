#pragma once
/**************************************************************************************************
 * Ich muss mir jetzt hier mal ein paar Dinge aufschreiben.
 * Das soll eine Art Visualisierung eines Optimierungsproblems werden.
 * Ich möchte die Datenpakete meiner Musik so im Speicher an den Player "anflanschen", dass mög-
 * lichst keine sog. Page Boundaries überquert werden.  Sicher funktioniert es Anfangs noch sehr gut
 * für die ersten Pages Lösungen zu finden.  Aber irgendwann muss das Backtracking auch abgebrochen
 * werden (oder findet in der Tat keine Lösung!), dann kommen die Prioritäten ins Spiel.
 *
 * Aber erstmal sollte es um den grundsätzlichen Algorithmus gehen:
 * - Ausgangspunkt: es gibt eine Liste mit Datenblöcken - genannt "chunks"
 *                  es gibt eine Anfangsposition im Speicher, wo dieser Daten abgelegt werden sollen
 * - Ziel: die Datenblöcke so anordnen, dass kein Datenblock eine Seitengrenze überschreitet.
 * - Vorgehensweise:
 *  -   Erstelle eine neue Liste "vorrat".  Sortiere die Chunk-IDs der Größe nach in diese Liste ein
 *  -   lege das Zwischenziel fest: der kleinste verfügbare Speicherblock (i.d.R. der Erste)
 *      Es ist wichtig, den kleinsten Block zuerst zu füllen.  Denn je weniger Platz vorhanden ist,
 *      desto besser ist eine große Auswahl an unterschiedlichen Blockgrößen.
 *  ->  Zwischenziel-Schleife:   Jedes Zwischenziel bekommt eine Lösung: List( ChunkIDs ), die kann
 *                               auch leer sein ... das wäre natürlich nicht so schön ...
 *  ->  irgendwann ist entweder der Vorrat leer -> Lösung gefunden
 *      ... oder man beginnt, sich milliausende Male im Kreis zu drehen.  Die Erfahrung zeigt, dass
 *      das Problem dieses Algorithmus seine Abbruchbedingung ist.  Sobald man nur eine Seite zurück
 *      "backtracked" ist noch alles OK, alle möglichen Varianten der verbliebenen Chunks werden
 *      durchprobiert.  Das Problem liegt beim Seitenwechsel, wenn eine Seite fertig befüllt ist.
 *      Dann wird der Vorratszeiger wieder auf das größte Element v_id=0 gestellt.  Ich vermute,
 *      dass durch dieses Verhalten sehr viele nicht-Lösungen mehrfach gefunden werden - wie schon
 *      angedeutet: der Algorthmus beginnt, sich irgendwie im Kreis zu drehen bzw. seinen Suchradius
 *      ins Unendliche zu erweitern.  Bisher habe ich nach ~300Mio Iterationen (auf den letzten
 *      beiden Pages) das Programm wieder abgebrochen.  Deshalb ist die aktuelle Vermutung: sobald
 *      auch die vorletzte Page zurück geschritten wurde ist davon auszugehen, dass keine Lösung mit
 *      vertretbarem Aufwand gefunden werden kann.
 *
 *  Wenn ich diesen Vorgang jetzt mal unterteilen möchte, sodass ich immer den nächsten Schritt nach
 *  einer Animation ausführe, dann ist auch die Geschwindigkeit in Echtzeit regelbar.  Weis ja nicht
 * ,ob es instant Animations gibt, aber das wäre jetzt ein Weg, den ich noch gehen möchte.
 *  =============
 *  Unterteilung:
 *  =============
 *  Member:     start_adresse = aktuelle Startadresse
 *  -------     bytes_seite = wie viele Bytes diese Seite braucht
 *              rest = wie viele Bytes mit der aktuellen (ggf. unfertigen) Lösung noch übrig sind
 *              ergebnis = List( chunk_ids ) ... wie der Name schon sagt ...
 *              vorrat = GeordneteListe( chunk_ids )
 *              chunks = ungeordnete Liste von Byte-Listen
 *              status = int - die aktuelle Seite | PAUSE (höchstes Bit gesetzt)
 *
 *  Funktionen:
 *  -----------
 *      nimm_vorrat: = animation->finished: kleine_schleife
 *      pack_vorrat: = animation->finished: zurueck
 *      starte_seite:
 *      -   bereitet die Variablen vor
 *      -   rückt die Anzeige zurecht -> Animation zu "kleine_schleife"
 *      kleine_schleife:
 *      -   solange wie ( v_id < vorrat.größe ) UND ( rest > 0 )
 *          -   vorrat[ v_id ].größe <= rest -> ja: nimm_vorrat( v_id ) -> Unterbrechung durch Ani
 *          +---------------------------------> nö: ++v_id             ->Endet in "kleine_schleife"
 *      -   Entscheidung:   fertig mit der Seite?
 *          -> Ja:  vorrat leer? -> Nein:   starte_nächste_seite
 *                               -> Ja:     loesung_gefunden
 *          -> Nö:  - ergebnis_sichern: ... wie der Name schon sagt. Kann ggf. weg gelassen/über-
 *                                          sprungen werden.
 *                  - pack_vorrat -> Unterbrechung durch Animation->Endet in "zurueck"
 *      zurueck:
 *      -   wenn ( ( v_id >= vorrat.größe ) && ( ergebnis.größe > 0 ) )
 *          -> Ja:  pack_vorrat -> Unterbrechung durch Animation->Endet in "zurueck"
 *          -> Nein:
 *              wenn ( v_id >= vorrat.größe ) -> Nein: Sprung zu "kleine_schleife"
 *              -> Ja: ist dies die erste Seite?
 *                -> Ja:    KEINE LÖSUNG GEFUNDEN!
 *                -> Nein:  eine Seite zurück gehen, pack_vorrat -> Unterbrechung durch Animation
 *                                                                  ->Endet in "zurueck"
 *      keine_loesung: blah
 *      loesung_gefunden: blubb
 *
 * Abbruchbedingung bisher: keine weiteren Möglichkeiten des Backtrackings.
 * 
 *  ->  sprich: alle Zigtilliarden Möglichkeiten wurden durchprobiert.  Nicht sequenziell mit
 *      heutiger Rechenleistung lösbar.  Obwohl wir das damals mal vermutet hatten.
 * 
 * Da ich keine Parallelisierbarkeit sehe, ist auch jetzt so nicht zu einer Lösung zu kommen.  Die
 * Erfahrung zeigt: ist eine Lösung möglich, wird sie durch die geordnete Herangehensweise auch
 * beachtlich schnell gefunden.
 * 
 * => Vermutlich sollte das erste Page-Backtracking als Zeichen gesehen werden, dass die Verteilung
 *    der restlichen Datenblöcke ungeeignet ist, und anstelle zur vorherigen Seite zurück zu tracken
 *    suche ich mir einen Vorgang, der mehr Erfolg verspricht.
 * 
 * -  Beispielsweise der letzte "Größenschritt" - die erste größte Speicherseite, die befüllt wurde.
 *    Wenn das Backtracking soweit zurück geht, dass ich an der Stelle weiter tracke, wo für die
 *    verbleibenden Seiten noch die größte Auswahl bestand, habe ich eine Einschränkung der mög-
 *    lichen Ergebnisse getroffen, allerdings werden sich die ursprünglichen Ergebnisse auch wieder
 *    zeigen.  Ich zweifele an dieser Idee ...
 * 
 * -  Vielleicht sollte ich beim Rückschritt über eine Seite nicht mit dem letzten Element anfangen,
 *    es auszutauschen, sondern gleich die Lösung bis zum 1. Chunk der vorigen Seite zurück treiben?
 * 
 * Fakt ist: der Algo läuft und läuft.  Ich habe jetzt in einer Debugging-Session nach 291Mio
 * Iterationen mal das Backtracking auf die vorletzte Seite miterlebt - der Algo tauscht weiter
 * fröhlich aus, was geht, um eine Seite zu füllen.  Es sind halt nur so verdammt viele
 * Möglichkeiten ...
 **************************************************************************************************/
#include "Stage.h"
#include "ui_PackerTester.h"

#include <QMainWindow>
#include <QStyle>

class QAbstractAnimation;
class QParallelAnimationGroup;

class PackerTester
	: public QMainWindow
	, public Ui::ChunkDistributorVisualizer
{
	Q_OBJECT

  public:
	PackerTester( QWidget *parent = nullptr );
	~PackerTester();
	bool isPaused() const { return state & pause; }
	bool eventFilter( QObject *o, QEvent *e ) override;


  public slots:
	void on_cb_tune_currentIndexChanged( int i );
	void on_button_clicked();
	void on_sl_spd_valueChanged( int );

	void on_bg_idPressed( int button_id );
	void on_bg_idReleased( int button_id );

	void rebuild_results();
	void clear_data();

	void naechste_seite();
	void finde_chunks();
	void zurueck();
	void keine_loesung();
	void loesung_gefunden();

  protected:
	void abschluss( QString ButtonText, QString MSG_Tx );
	void timerEvent( QTimerEvent *event ) override;

	typedef enum { nothing, iterate, backtrack, page_solved, solved, not_solved } continuation;

	void finde_position( ChunkObj *co );
	void nimm_vorrat();
	void pack_vorrat();
	void aktualisiere_anzeige();
	void tabs_einschalten();
	void warte( continuation );

  private slots:
	void machen( continuation );
	void do_pause( continuation );

  signals:
	void mache( continuation );

  private:
	// Daten
	QList< ChunkObj * >						daten;
	QList< QPair< quint16, QList< int > > > ergebnisse;
	QList< int >							vorrat;
	QList< Stage * >						seiten;
	Chunks								   *chunk_mgr{ nullptr };

	static const uint						pause			= 1 << 31;
	static const uint						pause_requested = 1 << 30;
	uint									state{ 0 };
	int										v_id{ 0 }, s_id{ -1 }, rest{ 0 };
	qint64									it_cnt{ 0 }, lbn;
	continuation							weiter_mit{ nothing };
	bool									boosted{ false }, animated{ true }, zoomed{ false };
	quint16									adr0{ 0 };
};
