//
// C++ Implementation: planreader_lua
//
// Description: 
//
//
// Author: Pierre Marchand <pierremarc@oep-h.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "planreader_lua.h"

#include <stdexcept>
#include <iostream>

// Note: this is *not* lua.hpp shipped with lua 5.1, it's a wrapper
// header we use to handle version differences between lua 5.1 and lua
// 5.0 .
#include "lua_compat.h"

LuaMachina::LuaMachina()
{
	/* Init the Lua interpreter */
	L = imp_lua_open();
	if (!L)
	{
		throw std::runtime_error("Whoops! Failed to open lua!");
	}

	/* Init the Lua libraries we want users to have access to.
	* Note that the `os' and `io' libraries MUST NOT be included,
	* as providing access to those libraries to the user would
	* make running plan files unsafe. */
	luaopen_base(L);
	luaopen_table(L);
	luaopen_string(L);
	luaopen_math(L);
}

LuaMachina::~LuaMachina()
{
	lua_close(L);
}

PlanReader_Lua::PlanReader_Lua(const std::string & planfile, PoDoFo::Impose::ImpositionPlan * ip)
{
// 	std::cerr<<"PlanReader_Lua::PlanReader_Lua "<< planfile <<std::endl;
	plan = ip;
	
	lua_pushcfunction(L.State(), &PlanReader_Lua::PushRecord);
	lua_setglobal(L.State(), "PushRecord");
	
	lua_pushlightuserdata(L.State(), static_cast<void*>(this));
	lua_setglobal(L.State(), "This"); 
	
	setNumber("PageCount", plan->sourceVars.PageCount);
	setNumber("SourceWidth", plan->sourceVars.PageWidth );
	setNumber("SourceHeight", plan->sourceVars.PageHeight);
	
	// imp_lua_dofile is a wrapper around luaL_dofile for Lua 5.0/5.1 compat.
	if(imp_lua_dofile(L.State(), planfile.c_str()))
	{
		std::cerr<<"Unable to process Lua script:\"" <<lua_tostring(L.State(), -1)<<"\""<<std::endl ;
	}
	else // if not reached, the plan remains invalid
	{
		if(hasGlobal("PageWidth"))
			plan->setDestWidth(getNumber("PageWidth"));
		if(hasGlobal("PageHeight"))
			plan->setDestHeight(getNumber("PageHeight"));
		if(hasGlobal("Scale"))
			plan->setScale(getNumber("Scale"));
		if(hasGlobal("BoundingBox"))
			plan->setBoundingBox(getString("BoundingBox"));
	}
	
}

PlanReader_Lua::~ PlanReader_Lua()
{ }

int PlanReader_Lua::PushRecord ( lua_State * L )
{
	/* TODO: check stack for space! 
	I would be glad to do that, but I don’t know how - pm
	*/
	if ( ! ( lua_isnumber ( L, 1 ) &&
	         lua_isnumber ( L, 2 ) &&
	         lua_isnumber ( L, 3 ) &&
	         lua_isnumber ( L, 4 ) &&
	         lua_isnumber ( L, 5 ) ) )
	{
		throw std::runtime_error ( "One or more arguments to PushRecord were not numbers" );
	}

	/* Get the instance of the reader which runs the script */
	lua_getglobal ( L , "This" );
	if ( ! lua_islightuserdata ( L, -1 ) )
		throw std::runtime_error ( "\"This\" is not valid" ); // ;-)
	PlanReader_Lua *that = static_cast<PlanReader_Lua*> ( lua_touserdata ( L, -1 ) );
	lua_pop ( L, 1 );

// 	std::cerr<<"PlanReader_Lua::PushRecord "<<lua_tonumber ( L, 1 )<<" "<<lua_tonumber ( L, 2 )<<" "<<lua_tonumber ( L, 3 )<<" "<<lua_tonumber ( L, 4 )<<" "<<lua_tonumber ( L, 5 )<<std::endl;
	
	/* and add a new record to it */
	PoDoFo::Impose::PageRecord P(
	                            lua_tonumber ( L, 1 ),
	                            lua_tonumber ( L, 2 ),
	                            lua_tonumber ( L, 3 ),
	                            lua_tonumber ( L, 4 ),
	                            lua_tonumber ( L, 5 ));
	if(P.isValid())
		that->plan->push_back ( P );

	lua_pop ( L,  5 );
	
	return 0;
}

double PlanReader_Lua::getNumber(const std::string & name)
{
	lua_getglobal(L.State(), name.c_str());
	if (!lua_isnumber(L.State(), -1))
	{
		std::string errString = name + " is non-number";
		throw std::runtime_error(errString.c_str());
	}
	double d = lua_tonumber(L.State(), -1);
	lua_pop(L.State(), 1);
	return d;
}

void PlanReader_Lua::setNumber(const std::string & name, double value)
{
	lua_pushnumber(L.State(), value);
	lua_setglobal(L.State(), name.c_str()); /* pops stack */
}

bool PlanReader_Lua::hasGlobal(const std::string & name)
{
	bool ret(true);
	lua_getglobal(L.State(), name.c_str());
	if(lua_isnil(L.State(), -1) > 0)
	{
		ret = false;
	}
	lua_pop(L.State(), 1);
	return ret;
}

std::string PlanReader_Lua::getString(const std::string& name)
{
	lua_getglobal(L.State(), name.c_str());
	if (!lua_isstring(L.State(), -1))
	{
		std::string errString = name + " is non-string";
		throw std::runtime_error(errString.c_str());
	}
	std::string s( lua_tostring(L.State(), -1) );
	lua_pop(L.State(), 1);
	return s;
}

