/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UNCOMPRESS_H
#define UNCOMPRESS_H

#include <podofo/podofo.h>

class UnCompress
{
public:
    UnCompress();
    ~UnCompress();

    void Init(const std::string_view& input, const std::string_view& output);

private:
    void UncompressObjects();

private:
    PoDoFo::PdfMemDocument* m_document;
};

#endif // UNCOMPRESS_H
