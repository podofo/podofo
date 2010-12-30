-- /***************************************************************************
--  *   Copyright (C) 2010 by Dominik Seichter                                *
--  *                         domseichter@web.de                              *
--  *                         Stefan  Huber                                   *
--  *                         sh@signalwerk.ch                                *
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
function set_stroking_color_rgb (r,g,b)
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
function set_non_stroking_color_rgb (r,g,b)
   -- Handle stroking and non-stroking rgb values in the same way
   local c = set_stroking_color_rgb (r,g,b)
   return c
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

-- This method converts a given RGB color to CMYK
-- The conversion is like a maximal 
-- under color removal
-- http://en.wikipedia.org/wiki/Under_color_removal
-- according to PoDoFo PdfColor::ConvertToCMYK()
--
-- @param a rgb color object
-- @returns the new CMYK color
function  ConvertRGBtoCMYK(r,g,b)
   -- local k = math.min( 1.0-r, math.min( 1.0-g, 1.0-b ) )
   local k = math.min( 1.0-r, 1.0-g, 1.0-b )

   local c = 0.0
   local m = 0.0
   local y = 0.0
   if k < 1.0 then
      c = (1.0 - r   - k) / (1.0 - k)
      m = (1.0 - g - k) / (1.0 - k)
      y = (1.0 - b  - k) / (1.0 - k)
   end

   return { c, m, y, k }
end

-- This method converts a given RGB color to Gray
-- according to PoDoFo PdfColor::ConvertToGrayScale()
--
-- @param a rgb color object
-- @returns the new GrayScale color
function  ConvertRGBtoGrayScale(r,g,b)
   return { 0.299*r + 0.587*g + 0.114*b }
end

-- Check if the given Color is nearly Gray
-- IsNearlyGray(0.126,0.127,0.128, 0.002) = true
-- IsNearlyGray(0.125,0.127,0.128, 0.002) = false
--
-- @param a rgb color object plus a tolerance-value
-- @returns true or false
function  IsNearlyGray(r,g,b,t)
   local min = math.min( r,g,b )
   local max = math.max( r,g,b )

   if max - min <= t and  max - min >= -t then
      return true
   else 
      return false
   end
end

-- Check if the given RGB Color1 is nearly RGB Color2
--
-- @param two rgb colors plus a tolerance-value
-- @returns true or false
function  IsNearlyTheSameRGB(r1,g1,b1, r2,g2,b2, t)
   local rMin = math.min( r1,r2 )
   local rMax = math.max( r1,r2 )
   local rSame = false

   local gMin = math.min( g1,g2 )
   local gMax = math.max( g1,g2 )
   local gSame = false

   local bMin = math.min( b1,b2 )
   local bMax = math.max( b1,b2 )
   local bSame = false

   if rMax - rMin <= t and  rMax - rMin >= -t then
      rSame = true
   end
   
   if gMax - gMin <= t and  gMax - gMin >= -t then
      gSame = true
   end
   
   if bMax - bMin <= t and  bMax - bMin >= -t then
      bSame = true
   end
   
   if rSame and gSame and bSame then
      return true
   else
      return false
   end
end
