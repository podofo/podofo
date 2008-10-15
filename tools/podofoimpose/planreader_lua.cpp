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

LuaMachina::LuaMachina()
{
// 	std::cerr<<"LuaMachina::LuaMachina"<<std::endl;
// 	luaopen_io(L);
// 	luaopen_base(L);
// 	luaopen_table(L);
// 	luaopen_string(L);
// 	luaopen_math(L);
	int error;

	/* Init the Lua interpreter */
	L = lua_open();
	if (!L)
	{
		throw std::runtime_error("Whoops! Failed to open lua!");
	}

	/* Init the Lua libraries we want users to have access to.
	* Note that the `os' and `io' libraries MUST NOT be included,
	* as providing access to those libraries to the user would
	* make running plan files unsafe. */
	error = luaopen_base(L);
// 	if (error)
	/// Found something weird in lua code source. And it seems it’s same for all luaopen_*
// 	LUALIB_API int luaopen_base (lua_State *L) {
// 		base_open(L);
// 		luaL_register(L, LUA_COLIBNAME, co_funcs);
// 		return 2; <- HERE !!!
// 	}

// 	{
// 		lua_close(L);
// 		throw std::runtime_error("Whoops! Failed top init lua base libs.");
// 	}
	error = luaopen_table(L);
// 	if (error)
// 	{
// 		lua_close(L);
// 		throw std::runtime_error("Whoops! Failed top init lua table libs.");
// 	}
	error = luaopen_string(L);
// 	if (error)
// 	{
// 		lua_close(L);
// 		throw std::runtime_error("Whoops! Failed top init lua string libs.");
// 	}
	error = luaopen_math(L);
// 	if (error)
// 	{
// 		lua_close(L);
// 		throw std::runtime_error("Whoops! Failed top init lua math libs.");
// 	}
}

LuaMachina::~LuaMachina()
{
// 	std::cerr<<"LuaMachina::~LuaMachina"<<std::endl;
	lua_close(L);
}

PlanReader_Lua::PlanReader_Lua(const std::string & planfile, PoDoFo::Impose::ImpositionPlan * ip)
{
// 	std::cerr<<"PlanReader_Lua::PlanReader_Lua "<< planfile <<std::endl;
	L = new LuaMachina;
	plan = ip;
	
	lua_pushcfunction(L->State(), &PlanReader_Lua::PushRecord);
	lua_setglobal(L->State(), "PushRecord");
	
	lua_pushlightuserdata(L->State(), static_cast<void*>(this));
	lua_setglobal(L->State(), "This"); 
	
	setNumber("PageCount", plan->sourceVars.PageCount);
	setNumber("SourceWidth", plan->sourceVars.PageWidth );
	setNumber("SourceHeight", plan->sourceVars.PageHeight);
	
	if(luaL_dofile(L->State(), planfile.c_str()))
	{
		std::cerr<<"Unable to process Lua script:\"" <<lua_tostring(L->State(), -1)<<"\""<<std::endl ;
	}
	else // if not reached, the plan remains invalid
	{
		if(hasGlobal("PageWidth"))
			plan->setDestWidth(getNumber("PageWidth"));
		if(hasGlobal("PageHeight"))
			plan->setDestHeight(getNumber("PageHeight"));
		if(hasGlobal("Scale"))
			plan->setScale(getNumber("Scale"));
	}
	
}

PlanReader_Lua::~ PlanReader_Lua()
{
// 	std::cerr<<"PlanReader_Lua::~ PlanReader_Lua"<<std::endl;
	if(L)
		delete L;
}

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
	lua_getglobal(L->State(), name.c_str());
	if (!lua_isnumber(L->State(), -1))
	{
		std::string errString = name + " is non-number";
		throw std::runtime_error(errString.c_str());
	}
	double d = lua_tonumber(L->State(), -1);
	lua_pop(L->State(), 1);
	return d;
}

void PlanReader_Lua::setNumber(const std::string & name, double value)
{
	lua_pushnumber(L->State(), value);
	lua_setglobal(L->State(), name.c_str()); /* pops stack */
}

bool PlanReader_Lua::hasGlobal(const std::string & name)
{
	bool ret(true);
	lua_getglobal(L->State(), name.c_str());
	if(lua_isnil(L->State(), -1) > 0)
	{
		ret = false;
	}
	lua_pop(L->State(), 1);
	return ret;
}


