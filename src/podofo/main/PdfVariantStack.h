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
    friend class PdfContentsReader;

public:
    using Stack = std::vector<PdfVariant>;
    using iterator = Stack::const_reverse_iterator;
    using reverse_iterator = Stack::const_iterator;

public:
    void Push(const PdfVariant& var);
    void Push(PdfVariant&& var);
    void Pop();
    void Clear();
    unsigned GetSize() const;

public:
    const PdfVariant& operator[](size_t index) const;
    iterator begin() const;
    iterator end() const;
    reverse_iterator rbegin() const;
    reverse_iterator rend() const;
    size_t size() const;

private:
    Stack m_variants;
};

}

#endif // PDF_OPERATOR_STACK_H
