-- /***************************************************************************
--  *   Copyright (C) 2010 by Dominik Seichter                                *
--  *   domseichter@web.de                                                    *
--  *                                                                         *
--  *   This program is free software; you can redistribute it and/or modify  *
--  *   it under the terms of the GNU General Public License as published by  *
--  *   the Free Software Foundation; either version 2 of the License, or     *
--  *   (at your option) any later version.                                   *
--  *                                                                         *
--  *   This program is distributed in the hope that it will be useful,       *
--  *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
--  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
--  *   GNU General Public License for more details.                          *
--  *                                                                         *
--  *   You should have received a copy of the GNU General Public License     *
--  *   along with this program; if not, write to the                         *
--  *   Free Software Foundation, Inc.,                                       *
--  *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
--  ***************************************************************************/

function start_page (n) 
   print( "   -> Lua is parsing page:", n + 1)
end

function end_page (n) 
   -- Do nothing
end

function start_xobjext ()
   -- Do nothing
end

function end_xobject ()
   -- Do nothing
end

-- This method is called whenever a gray stroking color is set
-- using the 'G' PDF command.
--
-- @param a grayscale color object
-- @returns a new color which should be set instead (any colorspace)
function set_stroking_color_gray (g)
   -- Convert all gray values to RGB red
   local a = { 1.0,0.0,0.0 }
   return a
end

-- This method is called whenever a rgb stroking color is set
-- using the 'RG' PDF command.
--
-- @param a rgb color object
-- @returns a new color which should be set instead (any colorspace)
function set_stroking_color_rgb (r,g,n)
   -- convert all black rgb values to cmyk,
   -- leave other as they are
   if r == 0 and
      g == 0 and
      b == 0 then
      return { 0.0, 0.0, 0.0, 1.0 }
   else 
      return { r,g,b }
   end
end

-- This method is called whenever a cmyk stroking color is set
-- using the 'K' PDF command.
--
-- @param a cmyk color object
-- @returns a new color which should be set instead (any colorspace)
function set_stroking_color_cmyk (c,m,y,k)
   -- do not change color,
   -- just return input values again
   return { c,m,y,k  }
end

-- This method is called whenever a gray non-stroking color is set
-- using the 'g' PDF command.
--
-- @param a grayscale color object
-- @returns a new color which should be set instead (any colorspace)
function set_non_stroking_color_gray (g)
   -- do not change color,
   -- just return input values again
   return { g }
end

-- This method is called whenever a rgb stroking color is set
-- using the 'rg' PDF command.
--
-- @param a rgb color object
-- @returns a new color which should be set instead (any colorspace)
function set_non_stroking_color_rgb (r,g,n)
   -- Handle stroking and non-stroking rgb values in the same way
   return set_stroking_color_rgb (r,g,b)
end

-- This method is called whenever a cmyk non-stroking color is set
-- using the 'k' PDF command.
--
-- @param a cmyk color object
-- @returns a new color which should be set instead (any colorspace)
function set_non_stroking_color_cmyk (c,m,y,k)
   -- do not change color,
   -- just return input values again
   return { c,m,y,k }
end
