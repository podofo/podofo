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
	bool CompareBox(const PoDoFo::Rect& rect1, const PoDoFo::Rect& rect2);
	BoxSetter(const std::string_view& in, const std::string_view& out, const std::string_view& box, const PoDoFo::Rect& rect);

private:
    const std::string m_box;
    const PoDoFo::Rect m_rect;
};

#endif // BOXSETTER_H
