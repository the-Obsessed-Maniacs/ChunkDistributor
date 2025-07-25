/**************************************************************************************************
 * AlgoData: Typendeklarationen
 * ============================
 **************************************************************************************************/
#pragma once

#include <QList>
#include <QMap>
#include <QReadWriteLock>

namespace Algo
{
	struct AlgoPage_
	{
		quint16		 start_address, end_address, bytes_left, penalty;
		QList< int > selection;
		QList< int > solution;
	};

	using ResultCache = QMap< quint16, AlgoPage_ >;

	struct AlgoCom
	{
		ResultCache			   result;
		mutable QReadWriteLock lock;
		bool				   freshData{ false };
	};

#pragma region Data_Type_Definitions
	// Algo-State - can be namespace-global ...
	typedef enum : quint64 {
		// 4 MS_Bits für den Zustand des Threads:
		// - initialized, running, pause requested/paused
		deeper_calc = ( 1ull << 63 ), // There are no perfect solutions - 2nd Stage, of algorithm
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
	static constexpr quint16 currentPage( State s )
	{
		return ( s & page_mask ) >> page_bits;
	}
	static constexpr quint64 currentStep( State s )
	{
		return ( s & step_mask );
	}
	static constexpr State fromValues( quint64 bits, quint16 current_page, quint64 current_step )
	{
		return static_cast< State >( ( bits & bits_mask )
									 | ( static_cast< quint64 >( current_page ) << page_bits )
									 | ( current_step & step_mask ) );
	}
	static constexpr State &operator|=( State &a, const quint64 &b )
	{
		return a = static_cast< State >( a | b );
	}
	static constexpr State &operator&=( State &a, const quint64 &b )
	{
		return a = static_cast< State >( a & b );
	}

	union ChunkData
	{
		quint32 value;
		struct
		{
			quint16 size;
			quint16 prio;
		};
		ChunkData( quint16 s, quint16 p )
			: size( s )
			, prio( p )
		{}
	};
	union PageData
	{
		quint64 value;
		struct
		{
			quint16 start_addr, end_addr, bytes_left, penalty;
		};
		PageData( quint16 s, quint16 e )
			: start_addr( s )
			, end_addr( e )
			, bytes_left( e - s )
			, penalty( 0 )
		{}
		explicit PageData( void )
			: PageData( 0, 0 )
		{}
	};
	using ChunkList = QList< ChunkData >;
	using PageList	= QList< PageData >;

	struct AlgoPage
	{
		PageData	 _;
		QList< int > selection;
		QList< int > solution;
	};
	using PageMap = QMap< quint16, AlgoPage >;
#pragma endregion

} // namespace Algo
