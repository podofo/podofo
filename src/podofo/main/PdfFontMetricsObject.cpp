/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsObject.h"

#include <podofo/private/FreetypePrivate.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObject.h"
#include "PdfVariant.h"

using namespace PoDoFo;
using namespace std;

static PdfFontStretch stretchFromString(const string_view& str);

PdfFontMetricsObject::PdfFontMetricsObject(const PdfObject& font, const PdfObject* descriptor) :
    m_DefaultWidth(0),
    m_FontFileObject(nullptr),
    m_FontFileType(PdfFontFileType::Unknown),
    m_Length1(0),
    m_Length2(0),
    m_Length3(0)
{
    const PdfObject* obj;
    const PdfName& subType = font.GetDictionary().MustFindKey(PdfName::KeySubtype).GetName();

    // Widths of a Type 1 font, which are in thousandths
    // of a unit of text space
    m_Matrix = { 1e-3, 0.0, 0.0, 1e-3, 0, 0 };

    // /FirstChar /LastChar /Widths are in the Font dictionary and not in the FontDescriptor
    if (subType == "Type1" || subType == "Type3" || subType == "TrueType")
    {
        if (subType == "Type1")
        {
            m_FontFileType = PdfFontFileType::Type1;
        }
        else if (subType == "TrueType")
        {
            m_FontFileType = PdfFontFileType::TrueType;
        }
        else if (subType == "Type3")
        {
            // Type3 fonts don't have a /FontFile entry
            m_FontFileType = PdfFontFileType::Type3;
        }

        if (descriptor == nullptr && m_FontFileType == PdfFontFileType::Type3)
        {
            const PdfObject* obj;
            if ((obj = font.GetDictionary().FindKey("Name")) != nullptr)
                m_FontNameRaw = obj->GetName().GetString();

            if ((obj = font.GetDictionary().FindKey("FontBBox")) != nullptr)
                m_BBox = getBBox(*obj);
        }
        else
        {
            if ((obj = descriptor->GetDictionary().FindKey("FontName")) != nullptr)
                m_FontNameRaw = obj->GetName().GetString();

            if ((obj = descriptor->GetDictionary().FindKey("FontBBox")) != nullptr)
                m_BBox = getBBox(*obj);

            if (m_FontFileType == PdfFontFileType::Type1)
            {
                m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile");
            }
            else if (m_FontFileType == PdfFontFileType::TrueType)
            {
                m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile2");
                // Try to eagerly load a built-in CIDToGID map, if needed
                // NOTE: This has to be done after having retrieved
                // the font file object
                if (!font.GetDictionary().HasKey("Encoding"))
                    tryLoadBuiltinCIDToGIDMap();
            }

            if (m_FontFileType != PdfFontFileType::Type3 && m_FontFileObject == nullptr)
            {
                m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile3");
                if (m_FontFileObject != nullptr)
                {
                    auto fontFileSubtype = m_FontFileObject->GetDictionary().FindKeyAs<PdfName>(PdfName::KeySubtype);
                    if (m_FontFileType == PdfFontFileType::Type1)
                    {
                        if (fontFileSubtype == "Type1C")
                            m_FontFileType = PdfFontFileType::Type1CCF;
                        else if (fontFileSubtype == "OpenType")
                            m_FontFileType = PdfFontFileType::OpenType;
                    }
                    else if (m_FontFileType == PdfFontFileType::TrueType && fontFileSubtype == "OpenType")
                    {
                        m_FontFileType = PdfFontFileType::OpenType;
                    }
                }
            }

            m_DefaultWidth = descriptor->GetDictionary().FindKeyAs<double>("MissingWidth", 0);
        }

        // Type3 fonts have a custom /FontMatrix
        const PdfObject* fontmatrix = nullptr;
        if (m_FontFileType == PdfFontFileType::Type3 && (fontmatrix = font.GetDictionary().FindKey("FontMatrix")) != nullptr)
        {
            auto& fontmatrixArr = fontmatrix->GetArray();
            for (int i = 0; i < 6; i++)
                m_Matrix[i] = fontmatrixArr[i].GetReal();
        }

        auto widths = font.GetDictionary().FindKey("Widths");
        if (widths != nullptr)
        {
            auto& arrWidths = widths->GetArray();
            m_Widths.reserve(arrWidths.size());
            for (auto& obj : arrWidths)
                m_Widths.push_back(obj.GetReal() * m_Matrix[0]);
        }
    }
    else if (subType == "CIDFontType0" || subType == "CIDFontType2")
    {
        if (descriptor == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Missing descriptor for CID ont");

        const PdfObject* obj;
        if ((obj = descriptor->GetDictionary().FindKey("FontName")) != nullptr)
            m_FontNameRaw = obj->GetName().GetString();

        if ((obj = descriptor->GetDictionary().FindKey("FontBBox")) != nullptr)
            m_BBox = getBBox(*obj);

        if (subType == "CIDFontType0"
            && (m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile")) != nullptr)
        {
            m_FontFileType = PdfFontFileType::Type1;
        }
        else if (subType == "CIDFontType2"
            && (m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile2")) != nullptr)
        {
            m_FontFileType = PdfFontFileType::TrueType;

            const PdfObject* cidToGidMapObj;
            const PdfObjectStream* stream;
            if ((cidToGidMapObj = font.GetDictionary().FindKey("CIDToGIDMap")) != nullptr
                && (stream = cidToGidMapObj->GetStream()) != nullptr)
            {
                m_CIDToGIDMap.reset(new PdfCIDToGIDMap(PdfCIDToGIDMap::Create(*cidToGidMapObj, PdfGlyphAccess::Width | PdfGlyphAccess::FontProgram)));
            }
        }

        if (m_FontFileObject == nullptr)
        {
            m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile3");
            if (m_FontFileObject != nullptr)
            {
                auto fontFileSubtype = m_FontFileObject->GetDictionary().FindKeyAs<PdfName>(PdfName::KeySubtype);
                if (subType == "CIDFontType0")
                {
                    if (fontFileSubtype == "CIDFontType0C")
                        m_FontFileType = PdfFontFileType::CIDType1;
                    else if (fontFileSubtype == "OpenType")
                        m_FontFileType = PdfFontFileType::OpenType;
                }
                else if (subType == "CIDFontType2" && fontFileSubtype == "OpenType")
                {
                    m_FontFileType = PdfFontFileType::OpenType;
                }
            }
        }

        if (m_FontFileObject != nullptr)
        {
            m_Length1 = (unsigned)m_FontFileObject->GetDictionary().FindKeyAs<int64_t>("Length1", 0);
            m_Length2 = (unsigned)m_FontFileObject->GetDictionary().FindKeyAs<int64_t>("Length2", 0);
            m_Length3 = (unsigned)m_FontFileObject->GetDictionary().FindKeyAs<int64_t>("Length3", 0);
        }

        m_DefaultWidth = font.GetDictionary().FindKeyAs<double>("DW", 1000.0) * m_Matrix[0];
        auto widths = font.GetDictionary().FindKey("W");
        if (widths != nullptr)
        {
            // "W" array format is described in Pdf 32000:2008 "9.7.4.3
            // Glyph Metrics in CIDFonts"
            PdfArray widthsArr = widths->GetArray();
            unsigned pos = 0;
            while (pos < widthsArr.GetSize())
            {
                unsigned start = (unsigned)widthsArr[pos++].GetNumberLenient();
                PdfObject* second = &widthsArr[pos];
                if (second->IsReference())
                {
                    // second do not have an associated owner; use the one in pw
                    second = &widths->GetDocument()->GetObjects().MustGetObject(second->GetReference());
                    PODOFO_ASSERT(!second->IsNull());
                }
                if (second->IsArray())
                {
                    PdfArray arr = second->GetArray();
                    pos++;
                    unsigned length = start + arr.GetSize();
                    PODOFO_ASSERT(length >= start);
                    if (length > m_Widths.size())
                        m_Widths.resize(length, m_DefaultWidth);

                    for (unsigned i = 0; i < arr.GetSize(); i++)
                        m_Widths[start + i] = arr[i].GetReal() * m_Matrix[0];
                }
                else
                {
                    unsigned end = (unsigned)widthsArr[pos++].GetNumberLenient();
                    unsigned length = end + 1;
                    PODOFO_ASSERT(length >= start);
                    if (length > m_Widths.size())
                        m_Widths.resize(length, m_DefaultWidth);

                    double width = widthsArr[pos++].GetReal() * m_Matrix[0];
                    for (unsigned i = start; i <= end; i++)
                        m_Widths[i] = width;
                }
            }
        }
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, subType.GetEscapedName());
    }

    if (descriptor == nullptr)
    {
        // Add some sensible defaults
        m_FontFamilyName.clear();
        m_FontStretch = PdfFontStretch::Unknown;
        m_Weight = -1;
        m_Flags = PdfFontDescriptorFlags::Symbolic;
        m_ItalicAngle = 0;
        m_Ascent = 0;
        m_Descent = 0;
        m_Leading = -1;
        m_CapHeight = 0;
        m_XHeight = 0;
        m_StemV = 0;
        m_StemH = -1;
        m_AvgWidth = -1;
        m_MaxWidth = -1;
    }
    else
    {
        auto& dict = descriptor->GetDictionary();
        auto fontFamilyObj = dict.FindKey("FontFamily");
        if (fontFamilyObj != nullptr)
        {
            const PdfString* str;
            if (fontFamilyObj->TryGetString(str))
            {
                m_FontFamilyName = str->GetString();
            }
            else
            {
                const PdfName* name;
                if (fontFamilyObj->TryGetName(name))
                    m_FontFamilyName = name->GetString();
            }
        }
        auto stretchObj = dict.FindKey("FontStretch");
        if (stretchObj == nullptr)
        {
            m_FontStretch = PdfFontStretch::Unknown;
        }
        else
        {
            const PdfName* name;
            const PdfString* str;
            if (stretchObj->TryGetName(name))
                m_FontStretch = stretchFromString(name->GetString());
            else if (stretchObj->TryGetString(str))
                m_FontStretch = stretchFromString(name->GetString());
            else
                m_FontStretch = PdfFontStretch::Unknown;
        }

        // NOTE: Found a valid document with "/FontWeight 400.0" so just read the value as double
        m_Weight = static_cast<int>(dict.FindKeyAs<double>("FontWeight", -1));
        m_Flags = (PdfFontDescriptorFlags)dict.FindKeyAs<int64_t>("Flags", 0);
        m_ItalicAngle = static_cast<int>(dict.FindKeyAs<double>("ItalicAngle", 0));
        m_Ascent = dict.FindKeyAs<double>("Ascent", 0) * m_Matrix[3];
        m_Descent = dict.FindKeyAs<double>("Descent", 0) * m_Matrix[3];
        m_Leading = dict.FindKeyAs<double>("Leading", -1) * m_Matrix[3];
        m_CapHeight = dict.FindKeyAs<double>("CapHeight", 0) * m_Matrix[3];
        m_XHeight = dict.FindKeyAs<double>("XHeight", 0) * m_Matrix[3];
        // NOTE: StemV is measured horizzontally, StemH vertically
        m_StemV = dict.FindKeyAs<double>("StemV", 0) * m_Matrix[0];
        m_StemH = dict.FindKeyAs<double>("StemH", -1) * m_Matrix[3];
        m_AvgWidth = dict.FindKeyAs<double>("AvgWidth", -1) * m_Matrix[0];
        m_MaxWidth = dict.FindKeyAs<double>("MaxWidth", -1) * m_Matrix[0];
    }

    // Accorting to ISO 32000-1:2008, /FontName "shall be the
    // same as the value of /BaseFont in the font or CIDFont
    // dictionary that refers to this font descriptor".
    // We prioritize /BaseFont, over /FontName
    if ((obj = font.GetDictionary().FindKey("BaseFont")) != nullptr)
        m_FontName = obj->GetName().GetString();
    if (m_FontName.length() == 0)
        m_FontName = m_FontNameRaw;

    m_FontBaseName = PdfFont::ExtractBaseName(m_FontName, m_IsItalicHint, m_IsBoldHint);
    m_LineSpacing = m_Ascent + m_Descent;

    // Try to fine some sensible values
    m_UnderlineThickness = 1.0;
    m_UnderlinePosition = 0.0;
    m_StrikeOutThickness = m_UnderlinePosition;
    m_StrikeOutPosition = m_Ascent / 2.0;
}

string_view PdfFontMetricsObject::GetFontName() const
{
    return m_FontName;
}

std::string_view PdfFontMetricsObject::GetFontNameRaw() const
{
    return m_FontNameRaw;
}

string_view PdfFontMetricsObject::GetBaseFontName() const
{
    return m_FontBaseName;
}

string_view PdfFontMetricsObject::GetFontFamilyName() const
{
    return m_FontFamilyName;
}

PdfFontStretch PdfFontMetricsObject::GetFontStretch() const
{
    return m_FontStretch;
}

PdfFontFileType PdfFontMetricsObject::GetFontFileType() const
{
    return m_FontFileType;
}

void PdfFontMetricsObject::GetBoundingBox(vector<double>& bbox) const
{
    bbox = m_BBox;
}

unique_ptr<PdfFontMetricsObject> PdfFontMetricsObject::Create(const PdfObject& font, const PdfObject* descriptor)
{
    return unique_ptr<PdfFontMetricsObject>(new PdfFontMetricsObject(font, descriptor));
}

unsigned PdfFontMetricsObject::GetGlyphCount() const
{
    return (unsigned)m_Widths.size();
}

bool PdfFontMetricsObject::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (gid >= m_Widths.size())
    {
        width = -1;
        return false;
    }

    width = m_Widths[gid];
    return true;
}

bool PdfFontMetricsObject::HasUnicodeMapping() const
{
    return false;
}

bool PdfFontMetricsObject::TryGetGID(char32_t codePoint, unsigned& gid) const
{
    (void)codePoint;
    // NOTE: We don't (and we won't) support retrieval of GID
    // from loaded metrics froma a codepoint. If one just needs
    // to retrieve the width of a codepoint then one map the
    // codepoint to a CID and retrieve the width directly
    gid = { };
    return false;
}

PdfFontDescriptorFlags PdfFontMetricsObject::GetFlags() const
{
    return m_Flags;
}

double PdfFontMetricsObject::GetDefaultWidthRaw() const
{
    return m_DefaultWidth;
}

double PdfFontMetricsObject::GetLineSpacing() const
{
    return m_LineSpacing;
}

double PdfFontMetricsObject::GetUnderlinePosition() const
{
    return m_UnderlinePosition;
}

double PdfFontMetricsObject::GetStrikeOutPosition() const
{
    return m_StrikeOutPosition;
}

double PdfFontMetricsObject::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsObject::GetStrikeOutThickness() const
{
    return m_StrikeOutThickness;
}

double PdfFontMetricsObject::GetAscent() const
{
    return m_Ascent;
}

double PdfFontMetricsObject::GetDescent() const
{
    return m_Descent;
}

double PdfFontMetricsObject::GetLeadingRaw() const
{
    return m_Leading;
}

int PdfFontMetricsObject::GetWeightRaw() const
{
    return m_Weight;
}

double PdfFontMetricsObject::GetCapHeight() const
{
    return m_CapHeight;
}

double PdfFontMetricsObject::GetXHeightRaw() const
{
    return m_XHeight;
}

double PdfFontMetricsObject::GetStemV() const
{
    return m_StemV;
}

double PdfFontMetricsObject::GetStemHRaw() const
{
    return m_StemH;
}

double PdfFontMetricsObject::GetAvgWidthRaw() const
{
    return m_AvgWidth;
}

double PdfFontMetricsObject::GetMaxWidthRaw() const
{
    return m_MaxWidth;
}

double PdfFontMetricsObject::GetItalicAngle() const
{
    return m_ItalicAngle;
}

const Matrix2D& PdfFontMetricsObject::GetMatrix() const
{
    return m_Matrix;
}

bool PdfFontMetricsObject::getIsBoldHint() const
{
    return m_IsBoldHint;
}

bool PdfFontMetricsObject::getIsItalicHint() const
{
    return m_IsItalicHint;
}

datahandle PdfFontMetricsObject::getFontFileDataHandle() const
{
    const PdfObjectStream* stream;
    if (m_FontFileObject == nullptr || (stream = m_FontFileObject->GetStream()) == nullptr)
        return datahandle();

    return datahandle(std::make_shared<charbuff>(stream->GetCopy()));
}

const PdfCIDToGIDMapConstPtr& PdfFontMetricsObject::getCIDToGIDMap() const
{
    return m_CIDToGIDMap;
}

const PdfObject* PdfFontMetricsObject::GetFontFileObject() const
{
    return m_FontFileObject;
}

unsigned PdfFontMetricsObject::GetFontFileLength1() const
{
    return m_Length1;
}

unsigned PdfFontMetricsObject::GetFontFileLength2() const
{
    return m_Length2;
}

unsigned PdfFontMetricsObject::GetFontFileLength3() const
{
    return m_Length3;
}

vector<double> PdfFontMetricsObject::getBBox(const PdfObject& obj)
{
    vector<double> ret;
    ret.reserve(4);
    auto& arr = obj.GetArray();
    ret.push_back(arr[0].GetNumberLenient() * m_Matrix[0]);
    ret.push_back(arr[1].GetNumberLenient() * m_Matrix[3]);
    ret.push_back(arr[2].GetNumberLenient() * m_Matrix[0]);
    ret.push_back(arr[3].GetNumberLenient() * m_Matrix[3]);
    return ret;
}

PdfFontStretch stretchFromString(const string_view& str)
{
    if (str == "UltraCondensed")
        return PdfFontStretch::UltraCondensed;
    if (str == "ExtraCondensed")
        return PdfFontStretch::ExtraCondensed;
    if (str == "Condensed")
        return PdfFontStretch::Condensed;
    if (str == "SemiCondensed")
        return PdfFontStretch::SemiCondensed;
    if (str == "Normal")
        return PdfFontStretch::Normal;
    if (str == "SemiExpanded")
        return PdfFontStretch::SemiExpanded;
    if (str == "Expanded")
        return PdfFontStretch::Expanded;
    if (str == "ExtraExpanded")
        return PdfFontStretch::ExtraExpanded;
    if (str == "UltraExpanded")
        return PdfFontStretch::UltraExpanded;
    else
        return PdfFontStretch::Unknown;
}

void PdfFontMetricsObject::tryLoadBuiltinCIDToGIDMap()
{
    FT_Face face;
    if (TryGetOrLoadFace(face) && face->num_charmaps != 0)
    {
        CIDToGIDMap map;

        // ISO 32000-1:2008 "9.6.6.4 Encodings for TrueType Fonts"
        // "A TrueType font program’s built-in encoding maps directly
        // from character codes to glyph descriptions by means of an
        // internal data structure called a 'cmap' "
        FT_Error rc;
        FT_ULong code;
        FT_UInt index;

        rc = FT_Set_Charmap(face, face->charmaps[0]);
        CHECK_FT_RC(rc, FT_Set_Charmap);

        if (face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
        {
            code = FT_Get_First_Char(face, &index);
            while (index != 0)
            {
                // "If the font contains a (3, 0) subtable, the range of character
                // codes shall be one of these: 0x0000 - 0x00FF,
                // 0xF000 - 0xF0FF, 0xF100 - 0xF1FF, or 0xF200 - 0xF2FF"
                // NOTE: we just take the first byte
                map.insert({ (unsigned)(code & 0xFF), index });
                code = FT_Get_Next_Char(face, code, &index);
            }
        }
        else
        {
            code = FT_Get_First_Char(face, &index);
            while (index != 0)
            {
                map.insert({ (unsigned)code, index });
                code = FT_Get_Next_Char(face, code, &index);
            }
        }

        m_CIDToGIDMap.reset(new PdfCIDToGIDMap(std::move(map), PdfGlyphAccess::FontProgram));
    }
}
