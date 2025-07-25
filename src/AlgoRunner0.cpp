#include "AlgoRunner0.h"

using namespace Qt::StringLiterals;
using namespace Algo;

const QString AlgoRunner0::name_in_factory = u"basic implementation with dead pages."_s;

AlgoRunner0::AlgoRunner0()
	: Registrar()
{}

void AlgoRunner0::iterate() {}
