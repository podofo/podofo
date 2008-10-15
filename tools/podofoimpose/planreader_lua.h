//
// C++ Header: planreader_lua
//
// Description:
//
//
// Author: Pierre Marchand <pierremarc@oep-h.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef PLANREADERLUA_H
#define PLANREADERLUA_H

			 
#include "impositionplan.h"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <string>

class LuaMachina
{
		lua_State *L;
	public:
		LuaMachina();
		~LuaMachina();

		inline lua_State* State()
		{
			return L;
		}
		
};


// could be best to have a base class but we wont develop 36000
// readers after all.
class PlanReader_Lua
{
	public:
		PlanReader_Lua(const std::string & planfile, PoDoFo::Impose::ImpositionPlan* ip);
		~PlanReader_Lua();
		
		static int PushRecord(lua_State *L);
		
	private:
		
		LuaMachina *L;
		PoDoFo::Impose::ImpositionPlan* plan;
		
		/** Ask if a variable is available in script global scope */
		bool hasGlobal(const std::string& name);
				
		/** Get the value of the named global from the Lua environment */
		double getNumber(const std::string& name);
		/** Set a global to the passed value */
		void setNumber(const std::string& name, double value);
};

#endif
