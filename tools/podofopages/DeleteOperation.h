/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DELETE_OPERATION_H
#define DELETE_OPERATION_H

#include "Operation.h"

class DeleteOperation : public Operation
{
public:
    DeleteOperation(unsigned pageIndex);
    virtual ~DeleteOperation() { }

    virtual void Perform(PoDoFo::PdfDocument& doc);
    virtual std::string ToString() const;

private:
    unsigned m_pageIndex;
};

#endif // DELETE_OPERATION_H
