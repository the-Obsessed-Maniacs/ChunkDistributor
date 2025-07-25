#pragma once

#include "AlgoFactory.h"

#define CommandWURST( T )                                                                          \
	class CommandWurst                                                                             \
	{                                                                                              \
		Q_OBJECT                                                                                   \
	}
#undef CommandWURST

class AlgoRunner0 : public Algo::AlgoWorker::Registrar< AlgoRunner0 >
{
	Q_OBJECT

  public:
	static const QString name_in_factory;
	AlgoRunner0();
	virtual ~AlgoRunner0() = default;

  public slots:
	void iterate() override;
};
