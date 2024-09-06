/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_OPERATOR_STACK_H
#define PDF_OPERATOR_STACK_H

#include "PdfVariant.h"

namespace PoDoFo {

class PODOFO_API PdfVariantStack final
{
    friend class PdfContentStreamReader;

public:
    using Stack = std::vector<PdfVariant>;
    using iterator = Stack::reverse_iterator;
    using reverse_iterator = Stack::iterator;
    using const_iterator = Stack::const_reverse_iterator;
    using const_reverse_iterator = Stack::const_iterator;

public:
    void Push(const PdfVariant& var);
    void Push(PdfVariant&& var);
    void Pop();
    void Clear();
    unsigned GetSize() const;

public:
    const PdfVariant& operator[](size_t index) const;
    PdfVariant& operator[](size_t index);
    iterator begin();
    iterator end();
    reverse_iterator rbegin();
    reverse_iterator rend();
    const_iterator begin() const;
    const_iterator end() const;
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;
    size_t size() const;

private:
    Stack m_variants;
};

}

#endif // PDF_OPERATOR_STACK_H
