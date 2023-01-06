/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "graphicsstack.h"

using namespace PoDoFo;

GraphicsStack::GraphicsStack()
{
    // Initialize graphicsstack with default graphicsstack
    TGraphicsStackElement element;
    m_stack.push(element);
}

GraphicsStack::~GraphicsStack()
{
}

void GraphicsStack::Push()
{
    PODOFO_RAISE_LOGIC_IF(m_stack.empty(), "Can push copy on graphicsstack! Stack is empty!");
    TGraphicsStackElement copy(m_stack.top());
    m_stack.push(copy);
}

void GraphicsStack::Pop()
{
    PODOFO_RAISE_LOGIC_IF(m_stack.empty(), "Can pop graphicsstack! Stack is empty!");
    m_stack.pop();
}

GraphicsStack::TGraphicsStackElement& GraphicsStack::GetCurrentGraphicsState()
{
    PODOFO_RAISE_LOGIC_IF(m_stack.empty(), "Can get current graphicsstate!");
    return m_stack.top();
}
