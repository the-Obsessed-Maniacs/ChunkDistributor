#pragma once

#include "Algo.h"

using namespace Algo;

class AlgoRunner0 : public AlgoWorker::Registrar< AlgoRunner0 >
{
	Q_OBJECT

  public:
	static const QString name_in_factory;
	AlgoRunner0();
	virtual ~AlgoRunner0() = default;

  public slots:
	void iterate() override;

  protected:
	void iterate_complete();
};
