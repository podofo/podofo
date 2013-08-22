/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "luaconverter.h"

// Note: this is *not* lua.hpp shipped with lua 5.1, it's a wrapper
// header we use to handle version differences between lua 5.1 and lua
// 5.0 .
#include "lua_compat.h"

#include <stdexcept>
#include <iostream>

using namespace PoDoFo;

#define FUNCTION_START_PAGE "start_page"
#define FUNCTION_END_PAGE "end_page"
#define FUNCTION_START_XOBJECT "start_xobject"
#define FUNCTION_END_XOBJECT "end_xobject"
#define FUNCTION_SET_STROKING_GRAY "set_stroking_color_gray"
#define FUNCTION_SET_STROKING_RGB "set_stroking_color_rgb"
#define FUNCTION_SET_STROKING_CMYK "set_stroking_color_cmyk"
#define FUNCTION_SET_NON_STROKING_GRAY "set_non_stroking_color_gray"
#define FUNCTION_SET_NON_STROKING_RGB "set_non_stroking_color_rgb"
#define FUNCTION_SET_NON_STROKING_CMYK "set_non_stroking_color_cmyk"

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

LuaConverter::LuaConverter( const std::string & sLuaScript )
    : IConverter()
{
	if( imp_lua_dofile(m_machina.State(), sLuaScript.c_str()) )
	{
		std::cerr << "Unable to process Lua script:\"" 
                  << lua_tostring(m_machina.State(), -1)<<"\"" << std::endl;
	}
}

LuaConverter::~LuaConverter()
{
}

void LuaConverter::StartPage( PdfPage*, int nPageNumber )
{
    lua_getglobal( m_machina.State(), FUNCTION_START_PAGE );
    lua_pushnumber( m_machina.State(), static_cast<double>(nPageNumber) );
    // Lua 5.1 only
    //lua_pushinteger( m_machina.State(), nPageNumber );
    lua_call( m_machina.State(), 1, 0 );
}

void LuaConverter::EndPage( PdfPage*, int nPageNumber )
{
    lua_getglobal( m_machina.State(), FUNCTION_END_PAGE );
    lua_pushnumber( m_machina.State(), static_cast<double>(nPageNumber) );
    // Lua 5.1 only
    //lua_pushinteger( m_machina.State(), nPageNumber );
    lua_call( m_machina.State(), 1, 0 );
}

void LuaConverter::StartXObject( PdfXObject* )
{
    lua_getglobal( m_machina.State(), FUNCTION_START_XOBJECT );
    lua_call( m_machina.State(), 0, 0 );
}

void LuaConverter::EndXObject( PdfXObject* )
{
    lua_getglobal( m_machina.State(), FUNCTION_END_XOBJECT );
    lua_call( m_machina.State(), 0, 0 );
}

PdfColor LuaConverter::GetColorFromReturnValue(const char* pszFunctionName)
{
    int top;
    double value;
    std::vector<double> colors;
    size_t len;

    luaL_checktype(m_machina.State(), 1, LUA_TTABLE);
    len = imp_lua_getn( m_machina.State(), -1 );
    // Lua 5.1 only
    //len = lua_objlen( m_machina.State(), -1 );

    for( int i = 1; i <= static_cast<int>(len); i++ )
    {
        lua_rawgeti( m_machina.State(), -1, i );
        top = lua_gettop( m_machina.State() );
        value = static_cast<double>(lua_tonumber(m_machina.State(), top));
        lua_pop(m_machina.State(), 1);

        colors.push_back( value );
    }

    switch( len ) 
    {
        case 1:
            return PdfColor( colors[0] );
        case 3:
            return PdfColor( colors[0],
                             colors[1],
                             colors[2] );
        case 4:
            return PdfColor( colors[0],
                             colors[1],
                             colors[2],
                             colors[3] );
        default:
        {
            PdfError::LogMessage( eLogSeverity_Error, "Array length is %i returned by %s.\n", len, pszFunctionName );
            PODOFO_RAISE_ERROR_INFO( ePdfError_CannotConvertColor,
                                     "Arrays returned from Lua must be { g }, { r,g,b } or { c,m,y,k }!" );
            break;
        }
    }

    return PdfColor();
}

PdfColor LuaConverter::SetStrokingColorGray( const PdfColor & rColor )
{
    lua_getglobal( m_machina.State(), FUNCTION_SET_STROKING_GRAY );
    lua_pushnumber( m_machina.State(), rColor.GetGrayScale() );
    lua_call( m_machina.State(), 1, 1 );

    return this->GetColorFromReturnValue( FUNCTION_SET_STROKING_GRAY );
}

PdfColor LuaConverter::SetStrokingColorRGB( const PdfColor & rColor )
{
    lua_getglobal( m_machina.State(), FUNCTION_SET_STROKING_RGB );
    lua_pushnumber( m_machina.State(), rColor.GetRed() );
    lua_pushnumber( m_machina.State(), rColor.GetGreen() );
    lua_pushnumber( m_machina.State(), rColor.GetBlue() );
    lua_call( m_machina.State(), 3, 1 );

    return this->GetColorFromReturnValue( FUNCTION_SET_STROKING_RGB );
}

PdfColor LuaConverter::SetStrokingColorCMYK( const PdfColor & rColor )
{
    lua_getglobal( m_machina.State(), FUNCTION_SET_STROKING_CMYK );
    lua_pushnumber( m_machina.State(), rColor.GetCyan() );
    lua_pushnumber( m_machina.State(), rColor.GetMagenta() );
    lua_pushnumber( m_machina.State(), rColor.GetYellow() );
    lua_pushnumber( m_machina.State(), rColor.GetBlack() );
    lua_call( m_machina.State(), 4, 1 );

    return this->GetColorFromReturnValue( FUNCTION_SET_STROKING_CMYK );
}

PdfColor LuaConverter::SetNonStrokingColorGray( const PdfColor & rColor )
{
    lua_getglobal( m_machina.State(), FUNCTION_SET_NON_STROKING_GRAY );
    lua_pushnumber( m_machina.State(), rColor.GetGrayScale() );
    lua_call( m_machina.State(), 1, 1 );

    return this->GetColorFromReturnValue( FUNCTION_SET_NON_STROKING_GRAY );
}

PdfColor LuaConverter::SetNonStrokingColorRGB( const PdfColor & rColor )
{
    lua_getglobal( m_machina.State(), FUNCTION_SET_NON_STROKING_RGB );
    lua_pushnumber( m_machina.State(), rColor.GetRed() );
    lua_pushnumber( m_machina.State(), rColor.GetGreen() );
    lua_pushnumber( m_machina.State(), rColor.GetBlue() );
    lua_call( m_machina.State(), 3, 1 );

    return this->GetColorFromReturnValue( FUNCTION_SET_NON_STROKING_RGB );
}

PdfColor LuaConverter::SetNonStrokingColorCMYK( const PdfColor & rColor )
{
    lua_getglobal( m_machina.State(), FUNCTION_SET_NON_STROKING_CMYK );
    lua_pushnumber( m_machina.State(), rColor.GetCyan() );
    lua_pushnumber( m_machina.State(), rColor.GetMagenta() );
    lua_pushnumber( m_machina.State(), rColor.GetYellow() );
    lua_pushnumber( m_machina.State(), rColor.GetBlack() );
    lua_call( m_machina.State(), 4, 1 );

    return this->GetColorFromReturnValue( FUNCTION_SET_NON_STROKING_CMYK );
}

