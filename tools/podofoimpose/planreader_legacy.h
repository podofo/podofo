//
// C++ Interface: planreader_legacy
//
// Description: 
//
//
// Author: Pierre Marchand <pierremarc@oep-h.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PLANREADER_LEGACY_H
#define PLANREADER_LEGACY_H

#include "impositionplan.h"

#include <string>
#include <vector>



class PlanReader_Legacy
{
	public:
		PlanReader_Legacy(const std::string & plan, PoDoFo::Impose::ImpositionPlan* Imp);
		~PlanReader_Legacy(){}
	private:
		PoDoFo::Impose::ImpositionPlan* I;
		int sortLoop(std::vector<std::string>& memfile, int numline);
};

#endif
