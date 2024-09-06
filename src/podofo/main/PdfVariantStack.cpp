/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfVariantStack.h"

using namespace std;
using namespace PoDoFo;

void PdfVariantStack::Push(const PdfVariant& var)
{
    m_variants.push_back(var);
}

void PdfVariantStack::Push(PdfVariant&& var)
{
    m_variants.push_back(std::move(var));
}

void PdfVariantStack::Pop()
{
    m_variants.pop_back();
}

void PdfVariantStack::Clear()
{
    m_variants.clear();
}

unsigned PdfVariantStack::GetSize() const
{
    return (unsigned)m_variants.size();
}

const PdfVariant& PdfVariantStack::operator[](size_t index) const
{
    // Access elements from the end
    index = (m_variants.size() - 1) - index;
    if (index >= m_variants.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index {} is out of range", index);

    return m_variants[index];
}

PdfVariant& PdfVariantStack::operator[](size_t index)
{
    // Access elements from the end
    index = (m_variants.size() - 1) - index;
    if (index >= m_variants.size())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index {} is out of range", index);

    return m_variants[index];
}

PdfVariantStack::iterator PdfVariantStack::begin()
{
    // Iterate elements from the end in the regular iteration
    return m_variants.rbegin();
}

PdfVariantStack::iterator PdfVariantStack::end()
{
    // Iterate elements from the end in the regular iteration
    return m_variants.rend();
}

PdfVariantStack::reverse_iterator PdfVariantStack::rbegin()
{
    // Iterate elements from the end in the regular iteration
    return m_variants.begin();
}

PdfVariantStack::reverse_iterator PdfVariantStack::rend()
{
    // Iterate elements from the end in the regular iteration
    return m_variants.end();
}

PdfVariantStack::const_iterator PdfVariantStack::begin() const
{
    // Iterate elements from the end in the regular iteration
    return m_variants.rbegin();
}

PdfVariantStack::const_iterator PdfVariantStack::end() const
{
    // Iterate elements from the end in the regular iteration
    return m_variants.rend();
}

PdfVariantStack::const_reverse_iterator PdfVariantStack::rbegin() const
{
    // Iterate elements from the begin the reverse iteration
    return m_variants.begin();
}

PdfVariantStack::const_reverse_iterator PdfVariantStack::rend() const
{
    // Iterate elements from the begin the reverse iteration
    return m_variants.end();
}

size_t PdfVariantStack::size() const
{
    return m_variants.size();
}
