#include "AlgoRunner0.h"

using namespace Qt::StringLiterals;

const QString AlgoRunner0::name_in_factory = u"basic implementation with dead pages."_s;

AlgoRunner0::AlgoRunner0()
	: Registrar()
{}

void AlgoRunner0::iterate()
{
	++iteration;
	do { // Außen: Backtracking, Innen: Number Selecting
		while ( a_id < avail.count()
				&& pages[ pageOrder[ p_id ] ]._.bytes_left > cur_btsleft_thresh )
			if ( pages[ pageOrder[ p_id ] ]._.bytes_left >= chunks[ avail[ a_id ] ].size )
			{
				if ( take_available() )
					if ( p_id < pageOrder.count() )
						return emit redo(); // Nächste Page -> neue Iteration!
					else return iterate_complete();
			} else ++a_id;
	} while ( !avail.isEmpty() && !make_available() );

	if ( avail.isEmpty() )
	{
		if ( p_id < pageOrder.count() ) emit page_finished( pageOrder[ p_id ] );
		emit final_solution( iteration, cnt_sel, cnt_unsel, 1.0 );
	} else {
		// Wir sind also nicht fertig geworden, keine Pause angefragt -> das heisst das
		// Backtracking hat einen Seitensprung gemacht.
		if ( pageOrder.isEmpty() || p_id >= pageOrder.count() )
			emit final_solution( iteration, cnt_sel, cnt_unsel, avail.isEmpty() );
		else emit redo();
	}
}

void AlgoRunner0::iterate_complete()
{
	// Die "final Solution" muss unvollständig sein, weil ja noch Chunks
	// available sind.  Hier ist noch kein "deeper shit" passiert, ich gebe
	// erst einmal eine Teillösung aus.  Muss ich natürlich berechnen,
	// wieviel Prozent gelöst sind ...
	int all = 0, left = 0;
	for ( int id = 0; id < chunks.count(); id++ )
	{
		all += chunks[ id ].size;
		if ( avail.contains( id ) ) left += chunks[ id ].size;
	}
	// Prozentual: wie viele Bytes sind unter der Haube?
	auto p_c = 1. - ( qreal( left ) / qreal( all ) );
	// Allerdings gäbe es noch andere Kriterien: Wieviele Chunks sind gefüllt?
	left = all = 0;
	for ( auto p : pages ) // nicht befüllbare Pages nicht zählen!
		if ( !( p.selection.count() == 1 && p.selection.first() == -1 ) )
		{
			left += p._.bytes_left;
			all += p._.end_addr - p._.start_addr;
		}
	auto		p_p = 1. - ( qreal( left ) / qreal( all ) );
	return emit final_solution( iteration, cnt_sel, cnt_unsel, ( p_c + p_p ) * .5 );
}
