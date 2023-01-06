/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef OPERATION_H
#define OPERATION_H

#include <podofo/podofo.h>

/**
 * Abstract base class for all operations
 * the podofopages can perform.
 *
 */
class Operation
{
public:
    virtual ~Operation() { }

    virtual void Perform(PoDoFo::PdfDocument& doc) = 0;

    virtual std::string ToString() const = 0;
};

#endif // OPERATION_H
