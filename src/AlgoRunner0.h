#pragma once

#include "Algo.h"

using namespace Algo;

class AlgoRunner0 : public Factory< WorkerBase >::Registrar< AlgoRunner0 >
{
	Q_OBJECT

  public:
	static const QString name_in_factory;

  public slots:
	void iterate() override;

  protected:
	void iterate_complete();
};
