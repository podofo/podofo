/**
 * SPDX-FileCopyrightText: (C) 2010 Pierre Marchand <pierre@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BOXSETTER_H
#define BOXSETTER_H

#include <podofo/podofo.h>
#include <string>

class BoxSetter
{
public:
	void SetBox(PoDoFo::PdfPage& page);
	bool CompareBox(const PoDoFo::PdfRect& rect1, const PoDoFo::PdfRect& rect2);
	BoxSetter(const std::string_view& in, const std::string& out, const std::string_view& box, const PoDoFo::PdfRect& rect);

private:
    const std::string m_box;
    const PoDoFo::PdfRect m_rect;
};

#endif // BOXSETTER_H
