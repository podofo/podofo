/**
 * SPDX-FileCopyrightText: (C) 2010 Pierre Marchand <pierre@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "boxsetter.h"

using namespace std;
using namespace PoDoFo;

BoxSetter::BoxSetter(const string_view& in, const string_view& out, const string_view& box, const Rect& rect)
    : m_box(box), m_rect(rect)
{
    PdfMemDocument source;
    source.Load(in);
    int pcount(source.GetPages().GetCount());
    for (int i = 0; i < pcount; ++i)
        SetBox(source.GetPages().GetPageAt(i));

    source.Save(out);
}

void BoxSetter::SetBox(PdfPage& page)
{
    PdfArray r;
    m_rect.ToArray(r);
    if (m_box.find("media") != string::npos)
        page.GetDictionary().AddKey("MediaBox", r);
    else if (m_box.find("crop") != string::npos)
        page.GetDictionary().AddKey("CropBox", r);
    else if (m_box.find("bleed") != string::npos)
        page.GetDictionary().AddKey("BleedBox", r);
    else if (m_box.find("trim") != string::npos)
        page.GetDictionary().AddKey("TrimBox", r);
    else if (m_box.find("art") != string::npos)
        page.GetDictionary().AddKey("ArtBox", r);

    // TODO check that box sizes are ordered
}

bool BoxSetter::CompareBox(const Rect &rect1, const Rect &rect2)
{
	return rect1.ToString() == rect2.ToString();
}
