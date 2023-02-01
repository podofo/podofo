/**
 * SPDX-FileCopyrightText: (C) 2007 Pierre Marchand <pierre@moulindetouvois.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "charpainter.h"

#include <vector>

using namespace std;

namespace
{
    // write the digits of `n' to `d'. The digits are ordered by increasing
    // place value.
    void digits(int n, vector<int>& d)
    {
        while (n > 0)
        {
            d.push_back(n % 10);
            n = n / 10;
        }
    }
}

void CharPainter::paint(ostream& s, int n, double size, double x, double y)
{
    // update working variables used by topleft() etc
    m_y = y;
    m_x = x;
    m_size = size;
    m_sh = m_size + m_y;
    m_midh = (m_size / 2.0) + m_y;
    m_sw = (m_size / 2.0) + m_x;

    switch (n)
    {
        case 1:
            topright(s);
            bottomright(s);
            break;
        case 2:
            top(s);
            topright(s);
            center(s);
            bottomleft(s);
            bottom(s);
            break;
        case 3:
            top(s);
            topright(s);
            bottomright(s);
            bottom(s);
            center(s);
            break;
        case 4:
            topleft(s);
            center(s);
            bottomright(s);
            topright(s);
            break;
        case 5:
            top(s);
            topleft(s);
            center(s);
            bottomright(s);
            bottom(s);
            break;
        case 6:
            top(s);
            topleft(s);
            center(s);
            bottomright(s);
            bottom(s);
            bottomleft(s);
            break;
        case 7:
            top(s);
            topright(s);
            bottomright(s);
            break;
        case 8:
            top(s);
            topleft(s);
            center(s);
            bottomright(s);
            bottom(s);
            bottomleft(s);
            topright(s);
            break;
        case 9:
            top(s);
            topleft(s);
            center(s);
            bottomright(s);
            bottom(s);
            topright(s);
            break;
        case 0:
            top(s);
            topleft(s);
            bottomright(s);
            bottom(s);
            bottomleft(s);
            topright(s);
            break;
    }
    s << "S\n";
}

void CharPainter::multipaint(ostream& s, int n, double size, double x, double y)
{
    vector<int> d;
    digits(n, d);

    vector<int>::const_reverse_iterator itEnd = d.rend();
    int i = (int)d.size() - 1;
    for (vector<int>::const_reverse_iterator it = d.rbegin(); it != itEnd; it++, i--)
        paint(s, *it, size, x + (size * i / 1.6), y);
}
