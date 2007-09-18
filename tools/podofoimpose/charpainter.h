/***************************************************************************
 *   Copyright (C) 2007 by Pierre Marchand   *
 *   pierre@moulindetouvois.com   *
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
#ifndef CHARPAINTER_H
#define CHARPAINTER_H

/**
	@author Pierre Marchand <pierre@moulindetouvois.com>
*/

#include <ostream>

class CharPainter{
public:
    CharPainter() { }
    ~CharPainter() { }

    void paint(std::ostream& s, int n, double size, double x, double y);
    // call paint() for each digit in the number `n'
    void multipaint(std::ostream& s, int n, double size, double x, double y);
private:
    // some simple helpers for writing points
    // TODO: nothrow annotations
    inline void wdir(std::ostream& s, double x1, double y1, double x2, double y2) const
    {
        s << x1 << ' ' << y1 << " m\n"
          << x2 << ' ' << y2 << " l\n";
    }
    inline void top(std::ostream& s) const { wdir(s, m_x, m_sh, m_sw, m_x); }
    inline void topright(std::ostream& s) const { wdir(s, m_sw, m_sh, m_sw, m_midh); }
    inline void bottomright(std::ostream& s) const { wdir(s, m_sw, m_midh, m_sw, m_y); }
    inline void bottom(std::ostream& s) const { wdir(s, m_x, m_y, m_sw, m_y); }
    inline void bottomleft(std::ostream&s) const { wdir(s, m_x, m_y, m_x, m_midh); }
    inline void topleft(std::ostream& s) const { wdir(s, m_x, m_midh, m_x, m_sh); }
    inline void center(std::ostream& s) const { wdir(s, m_x, m_midh, m_sw, m_midh); }

    // temporaries used by paint(...)
    double m_size;
    double m_x;
    double m_y;
    double m_sh;
    double m_sw;
    double m_midh;
};

#endif
