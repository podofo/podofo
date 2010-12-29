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

#include "graphicsstack.h"

using namespace PoDoFo;

GraphicsStack::GraphicsStack()
{
    // Initialize graphicsstack with default graphicsstack
    TGraphicsStackElement element;
    m_stack.push( element );
}

GraphicsStack::~GraphicsStack()
{
}

void GraphicsStack::Push()
{
    PODOFO_RAISE_LOGIC_IF( m_stack.empty(), "Can push copy on graphicsstack! Stack is empty!" );

    TGraphicsStackElement copy( m_stack.top() );
    m_stack.push( copy );
}

void GraphicsStack::Pop()
{
    PODOFO_RAISE_LOGIC_IF( m_stack.empty(), "Can pop graphicsstack! Stack is empty!" );

    m_stack.pop();
}

GraphicsStack::TGraphicsStackElement & GraphicsStack::GetCurrentGraphicsState()
{
    PODOFO_RAISE_LOGIC_IF( m_stack.empty(), "Can get current graphicsstate!" );
    return m_stack.top();
}
