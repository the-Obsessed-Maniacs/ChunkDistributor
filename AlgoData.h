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
	struct AlgoPage
	{
		quint16		 start_address, end_address, bytes_left, penalty;
		QList< int > solution;
		QList< int > bestSolution;
	};

	using ResultCache = QMap< quint16, AlgoPage >;

	struct AlgoCom
	{
		ResultCache			   result;
		mutable QReadWriteLock lock;
		bool				   freshData{ false };
	};
} // namespace Algo
