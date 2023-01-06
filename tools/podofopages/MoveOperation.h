/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MOVE_OPERATION_H
#define MOVE_OPERATION_H

#include "Operation.h"

class MoveOperation : public Operation
{
public:
    MoveOperation(int fromIndex, int toIndex);

    virtual ~MoveOperation() { }

    virtual void Perform(PoDoFo::PdfDocument& doc);
    virtual std::string ToString() const;

private:
    unsigned m_fromIndex;
    unsigned m_toIndex;
};

#endif // MOVE_OPERATION_H
