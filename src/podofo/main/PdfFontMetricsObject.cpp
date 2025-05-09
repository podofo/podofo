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

namespace PoDoFo
{
    struct PdfFontMetricsObject::Type3FontData
    {
        const PdfObject* CharProcsObj;
    };
}

using namespace PoDoFo;
using namespace std;

static PdfFontStretch stretchFromString(const string_view& str);

PdfFontMetricsObject::PdfFontMetricsObject(const PdfDictionary& fontDict,
        const PdfReference& fontRef, const PdfDictionary* descriptorDict) :
    m_SubsetPrefixLength(0),
    m_IsItalicHint(false),
    m_IsBoldHint(false),
    m_HasBBox(false),
    m_BBox{ },
    m_DefaultWidth(0),
    m_FontFileObject(nullptr),
    m_Type3FontData(nullptr),
    m_Length1(0),
    m_Length2(0),
    m_Length3(0)
{
    const PdfObject* obj;
    const PdfName& subType = fontDict.MustFindKey("Subtype").GetName();
    bool isSimpleFont;
    if (subType == "Type1")
    {
        m_FontType = PdfFontType::Type1;
        isSimpleFont = true;
    }
    else if (subType == "TrueType")
    {
        m_FontType = PdfFontType::TrueType;
        isSimpleFont = true;
    }
    else if (subType == "Type3")
    {
        m_FontType = PdfFontType::Type3;
        isSimpleFont = true;
    }
    else if (subType == "CIDFontType0")
    {
        m_FontType = PdfFontType::CIDCFF;
        isSimpleFont = false;
    }
    else if (subType == "CIDFontType2")
    {
        m_FontType = PdfFontType::CIDTrueType;
        isSimpleFont = false;
    }
    else
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, subType.GetString());

    // Set a default identity matrix. Widths are normally in
    // thousands of a unit of text space
    m_Matrix = { 1e-3, 0.0, 0.0, 1e-3, 0, 0 };

    // /FirstChar /LastChar /Widths are in the Font dictionary and not in the FontDescriptor
    double missingWidthRaw = 0;
    if (isSimpleFont)
    {
        if (m_FontType == PdfFontType::Type3)
        {
            // Type3 fonts don't have a /FontFile entry
            m_FontFileType = PdfFontFileType::Type3;
            m_Type3FontData = new Type3FontData{ };
        }

        const PdfObject* fontmatrix = nullptr;
        if (m_FontType == PdfFontType::Type3 && (fontmatrix = fontDict.FindKey("FontMatrix")) != nullptr)
        {
            // Type3 fonts have a custom /FontMatrix
            auto& fontmatrixArr = fontmatrix->GetArray();
            m_Matrix = Matrix::FromArray(fontmatrixArr);
        }

        if (descriptorDict == nullptr)
        {
            if (m_FontType == PdfFontType::Type3)
            {
                if ((obj = fontDict.FindKey("Name")) != nullptr)
                    m_FontNameRaw = obj->GetName().GetString();

                if ((obj = fontDict.FindKey("FontBBox")) != nullptr)
                {
                    m_BBox = getBBox(*obj);
                    m_HasBBox = true;
                }

                auto charProcs = fontDict.FindKeyAsSafe<const PdfDictionary*>("CharProcs");
                if (charProcs != nullptr)
                    m_Type3FontData->CharProcsObj = charProcs->GetOwner();
            }
            else
            {
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidFontData);
            }
        }
        else
        {
            if ((obj = descriptorDict->FindKey("FontName")) != nullptr)
                m_FontNameRaw = obj->GetName().GetString();

            if ((obj = descriptorDict->FindKey("FontBBox")) != nullptr)
            {
                m_BBox = getBBox(*obj);
                m_HasBBox = true;
            }

            if (m_FontType == PdfFontType::Type1)
            {
                m_FontFileObject = descriptorDict->FindKey("FontFile");
            }
            else if (m_FontType == PdfFontType::TrueType)
            {
                m_FontFileObject = descriptorDict->FindKey("FontFile2");
            }

            if (m_FontType != PdfFontType::Type3 && m_FontFileObject == nullptr)
                m_FontFileObject = descriptorDict->FindKey("FontFile3");

            missingWidthRaw = descriptorDict->FindKeyAsSafe<double>("MissingWidth", 0);
        }

        // Set the default width accordingly to possibly existing
        // /MissingWidth and /FontMatrix
        m_DefaultWidth = missingWidthRaw * m_Matrix[0];

        auto widthsObj = fontDict.FindKey("Widths");
        if (widthsObj != nullptr)
        {
            auto& arrWidths = widthsObj->GetArray();
            vector<double> widths(arrWidths.size());
            for (unsigned i = 0; i < arrWidths.GetSize(); i++)
                widths[i] = arrWidths[i].GetReal() * m_Matrix[0];

            SetParsedWidths(std::make_shared<vector<double>>(std::move(widths)));
        }
    }
    else
    {
        if (descriptorDict == nullptr)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Missing descriptor for CID ont");

        if ((obj = descriptorDict->FindKey("FontName")) != nullptr)
            m_FontNameRaw = obj->GetName().GetString();

        if ((obj = descriptorDict->FindKey("FontBBox")) != nullptr)
        {
            m_BBox = getBBox(*obj);
            m_HasBBox = true;
        }

        if (m_FontType == PdfFontType::CIDCFF)
        {
            m_FontFileObject = descriptorDict->FindKey("FontFile3");
            if (m_FontFileObject == nullptr)
                m_FontFileObject = descriptorDict->FindKey("FontFile");
        }
        else if (m_FontType == PdfFontType::CIDTrueType)
        {
            m_FontFileObject = descriptorDict->FindKey("FontFile2");
            if (m_FontFileObject == nullptr)
                m_FontFileObject = descriptorDict->FindKey("FontFile3");
        }

        if (m_FontFileObject != nullptr)
        {
            m_Length1 = (unsigned)m_FontFileObject->GetDictionary().FindKeyAsSafe<int64_t>("Length1", 0);
            m_Length2 = (unsigned)m_FontFileObject->GetDictionary().FindKeyAsSafe<int64_t>("Length2", 0);
            m_Length3 = (unsigned)m_FontFileObject->GetDictionary().FindKeyAsSafe<int64_t>("Length3", 0);
        }

        m_DefaultWidth = fontDict.FindKeyAsSafe<double>("DW", 1000.0) * m_Matrix[0];
        auto widthsObj = fontDict.FindKey("W");
        if (widthsObj != nullptr)
        {
            // "W" array format is described in Pdf 32000:2008 "9.7.4.3
            // Glyph Metrics in CIDFonts"
            auto& widthsArr = widthsObj->GetArray();
            unsigned pos = 0;
            vector<double> widths;
            while (pos < widthsArr.GetSize())
            {
                unsigned start = (unsigned)widthsArr[pos++].GetNumberLenient();
                auto second = &widthsArr[pos];
                if (second->IsReference())
                {
                    // second do not have an associated owner; use the one in pw
                    second = &widthsObj->GetDocument()->GetObjects().MustGetObject(second->GetReference());
                    PODOFO_ASSERT(!second->IsNull());
                }

                const PdfArray* arr;
                if (second->TryGetArray(arr))
                {
                    pos++;
                    unsigned length = start + arr->GetSize();
                    PODOFO_ASSERT(length >= start);
                    if (length > widths.size())
                        widths.resize(length, m_DefaultWidth);

                    for (unsigned i = 0; i < arr->GetSize(); i++)
                        widths[start + i] = (*arr)[i].GetReal() * m_Matrix[0];
                }
                else
                {
                    unsigned end = (unsigned)widthsArr[pos++].GetNumberLenient();
                    unsigned length = end + 1;
                    PODOFO_ASSERT(length >= start);
                    if (length > widths.size())
                        widths.resize(length, m_DefaultWidth);

                    double width = widthsArr[pos++].GetReal() * m_Matrix[0];
                    for (unsigned i = start; i <= end; i++)
                        widths[i] = width;
                }
            }

            SetParsedWidths(std::make_shared<vector<double>>(std::move(widths)));
        }
    }

    if (descriptorDict == nullptr)
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
        auto fontFamilyObj = descriptorDict->FindKey("FontFamily");
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
        auto stretchObj = descriptorDict->FindKey("FontStretch");
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

        int64_t num;
        if (descriptorDict->TryFindKeyAs("Flags", num))
            m_Flags = (PdfFontDescriptorFlags)num;

        if (!descriptorDict->TryFindKeyAs("ItalicAngle", m_ItalicAngle))
            m_ItalicAngle = numeric_limits<double>::quiet_NaN();

        if (descriptorDict->TryFindKeyAs("Ascent", m_Ascent))
            m_Ascent *= m_Matrix[3];
        else
            m_Ascent = numeric_limits<double>::quiet_NaN();

        // ISO 32000-2:2020 "The value shall be a negative number"
        if (descriptorDict->TryFindKeyAs("Descent", m_Descent) && m_Descent < 0)
            m_Descent *= m_Matrix[3];
        else
            m_Descent = numeric_limits<double>::quiet_NaN();

        if (descriptorDict->TryFindKeyAs("CapHeight", m_CapHeight))
            m_CapHeight *= m_Matrix[3];
        else
            m_CapHeight = numeric_limits<double>::quiet_NaN();

        // ISO 32000-2:2020 "The value shall be a negative number"
        // NOTE: StemV is measured horizzontally, StemH vertically
        if (descriptorDict->TryFindKeyAs("StemV", m_StemV) && m_StemV >= 0)
            m_StemV *= m_Matrix[0];
        else
            m_StemV = numeric_limits<double>::quiet_NaN();

        // NOTE1: If missing we store the following values as
        // negative. Default value handling is done in PdfFontMetrics
        // NOTE2: Found a document with "/FontWeight 400.0"
        // which is parsed correctly by Adobe Acrobat so just
        // read the value as double
        m_Weight = static_cast<short>(descriptorDict->FindKeyAsSafe<double>("FontWeight", -1));
        m_Leading = descriptorDict->FindKeyAsSafe<double>("Leading", -1) * m_Matrix[3];
        m_XHeight = descriptorDict->FindKeyAsSafe<double>("XHeight", -1) * m_Matrix[3];
        m_StemH = descriptorDict->FindKeyAsSafe<double>("StemH", -1) * m_Matrix[3];
        m_AvgWidth = descriptorDict->FindKeyAsSafe<double>("AvgWidth", -1) * m_Matrix[0];
        m_MaxWidth = descriptorDict->FindKeyAsSafe<double>("MaxWidth", -1) * m_Matrix[0];
    }

    // According to ISO 32000-2:2020, /FontName "shall be the
    // same as the value of /BaseFont in the font or CIDFont
    // dictionary that refers to this font descriptor".
    // We prioritize /BaseFont, over /FontName
    if ((obj = fontDict.FindKey("BaseFont")) != nullptr)
        m_FontName = obj->GetName().GetString();

    if (m_FontName.empty())
    {
        if (m_FontNameRaw.empty())
        {
            if (m_FontFamilyName.empty())
            {
                // Set a fallback name
                m_FontName = utls::Format("Font{}_{}",
                    fontRef.ObjectNumber(),
                    fontRef.GenerationNumber());
            }
            else
            {
                m_FontName = m_FontFamilyName;
            }
        }
        else
        {
            m_FontName = m_FontNameRaw;
        }
    }

    m_LineSpacing = m_Ascent + m_Descent;

    // Try to fine some sensible values
    m_UnderlineThickness = 1.0;
    m_UnderlinePosition = 0.0;
    m_StrikeThroughThickness = m_UnderlinePosition;
    m_StrikeThroughPosition = m_Ascent / 2.0;
}

string_view PdfFontMetricsObject::GetFontName() const
{
    return m_FontName;
}

string_view PdfFontMetricsObject::GetFontNameRaw() const
{
    return m_FontNameRaw;
}

string_view PdfFontMetricsObject::GetBaseFontName() const
{
    const_cast<PdfFontMetricsObject&>(*this).processFontName();
    return m_FontBaseName;
}

PdfFontType PdfFontMetricsObject::GetFontType() const
{
    return m_FontType;
}

string_view PdfFontMetricsObject::GetFontFamilyName() const
{
    return m_FontFamilyName;
}

unsigned char PdfFontMetricsObject::GetSubsetPrefixLength() const
{
    const_cast<PdfFontMetricsObject&>(*this).processFontName();
    return m_SubsetPrefixLength;
}

PdfFontStretch PdfFontMetricsObject::GetFontStretch() const
{
    return m_FontStretch;
}

PdfFontFileType PdfFontMetricsObject::GetFontFileType() const
{
    if (m_FontFileType.has_value())
        return *m_FontFileType;

    auto face = GetFaceHandle();
    PdfFontFileType type;
    if (face == nullptr || !FT::TryGetFontFileFormat(face, type))
        type = PdfFontFileType::Unknown;

    const_cast<PdfFontMetricsObject&>(*this).m_FontFileType = type;
    return type;
}

void PdfFontMetricsObject::ExportType3GlyphData(PdfDictionary& fontDict, cspan<std::string_view> glyphs) const
{
    if (m_FontFileType != PdfFontFileType::Type3 || m_Type3FontData->CharProcsObj == nullptr)
        return;

    auto& objects = fontDict.GetOwner()->MustGetDocument().GetObjects();
    auto& charProcs = objects.CreateDictionaryObject();
    if (glyphs.size() == 0)
    {
        charProcs = *m_Type3FontData->CharProcsObj;
    }
    else
    {
        auto& srcCharProcs = m_Type3FontData->CharProcsObj->GetDictionary();
        auto& dstCharProcs = charProcs.GetDictionary();
        for (unsigned i = 0; i < glyphs.size(); i++)
        {
            auto obj = srcCharProcs.FindKey(glyphs[i]);
            if (obj->GetStream() == nullptr)
            {
                // Create a object with a dummy stream
                auto& newObject = objects.CreateDictionaryObject();
                newObject.ForceCreateStream();
                obj = &newObject;
            }

            dstCharProcs.AddKeyIndirect(glyphs[i], *obj);
        }
    }
    fontDict.AddKeyIndirect("CharProcs"_n, charProcs);
}

unsigned PdfFontMetricsObject::GetGlyphCountFontProgram() const
{
    if (m_FontFileType == PdfFontFileType::Type3)
    {
        // This is interesting. /Type3 fonts:
        // - Don't have a /FontFile data where glyphs can can be read from;
        // - Glyphs are not random accessed by index but by glyph name
        // This means that we are in a situation similar to CID keyed fonts,
        // where we can't really random access glyphs in the storage.
        // Because glyph count from this instance will be mostly accessed
        // for metrics reading, which is allowed to span out of ranges
        // with default values, wel'll arbitrarily return the maximum possible
        // glyph count for Type3 fonts, which is limited to one-byte encodings.
        // Cross validation for glyphs data consitency will be performed at
        // a later stage
        return 255U;
    }

    return PdfFontMetricsBase::GetGlyphCountFontProgram();
}

unique_ptr<const PdfFontMetricsObject> PdfFontMetricsObject::Create(const PdfObject& font, const PdfDictionary* descriptorDict)
{
    return unique_ptr<const PdfFontMetricsObject>(new PdfFontMetricsObject(font.GetDictionary(), font.GetIndirectReference(), descriptorDict));
}

PdfFontMetricsObject::~PdfFontMetricsObject()
{
    delete m_Type3FontData;
}

unique_ptr<const PdfFontMetricsObject> PdfFontMetricsObject::Create(const PdfObject& font)
{
    return unique_ptr<const PdfFontMetricsObject>(new PdfFontMetricsObject(
        font.GetDictionary(), font.GetIndirectReference(),
        font.GetDictionary().FindKeyAsSafe<const PdfDictionary*>("FontDescriptor")));
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

bool PdfFontMetricsObject::TryGetFlags(PdfFontDescriptorFlags& value) const
{
    if (m_Flags == nullptr)
    {
        value = PdfFontDescriptorFlags::None;
        return false;
    }

    value = *m_Flags;
    return true;
}

bool PdfFontMetricsObject::TryGetBoundingBox(Corners& value) const
{
    value = m_BBox;
    return m_HasBBox;
}

bool PdfFontMetricsObject::TryGetItalicAngle(double& value) const
{
    if (std::isnan(m_ItalicAngle))
    {
        value = 0;
        return false;
    }

    value = m_ItalicAngle;
    return true;
}

bool PdfFontMetricsObject::TryGetAscent(double& value) const
{
    if (std::isnan(m_Ascent))
    {
        value = 0;
        return false;
    }

    value = m_Ascent;
    return true;
}

bool PdfFontMetricsObject::TryGetDescent(double& value) const
{
    if (std::isnan(m_Descent))
    {
        value = 0;
        return false;
    }

    value = m_Descent;
    return true;
}

bool PdfFontMetricsObject::TryGetCapHeight(double& value) const
{
    if (std::isnan(m_CapHeight))
    {
        value = 0;
        return false;
    }

    value = m_CapHeight;
    return true;
}

bool PdfFontMetricsObject::TryGetStemV(double& value) const
{
    if (std::isnan(m_StemV))
    {
        value = 0;
        return false;
    }

    value = m_StemV;
    return true;
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

double PdfFontMetricsObject::GetStrikeThroughPosition() const
{
    return m_StrikeThroughPosition;
}

double PdfFontMetricsObject::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsObject::GetStrikeThroughThickness() const
{
    return m_StrikeThroughThickness;
}

double PdfFontMetricsObject::GetLeadingRaw() const
{
    return m_Leading;
}

int PdfFontMetricsObject::GetWeightRaw() const
{
    return m_Weight;
}

double PdfFontMetricsObject::GetXHeightRaw() const
{
    return m_XHeight;
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

const Matrix& PdfFontMetricsObject::GetMatrix() const
{
    return m_Matrix;
}

bool PdfFontMetricsObject::IsObjectLoaded() const
{
    return true;
}

bool PdfFontMetricsObject::getIsBoldHint() const
{
    const_cast<PdfFontMetricsObject&>(*this).processFontName();
    return m_IsBoldHint;
}

bool PdfFontMetricsObject::getIsItalicHint() const
{
    const_cast<PdfFontMetricsObject&>(*this).processFontName();
    return m_IsItalicHint;
}

datahandle PdfFontMetricsObject::getFontFileDataHandle() const
{
    const PdfObjectStream* stream;
    if (m_FontFileObject == nullptr || (stream = m_FontFileObject->GetStream()) == nullptr)
        return datahandle();

    return datahandle(std::make_shared<charbuff>(stream->GetCopy()));
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

void PdfFontMetricsObject::processFontName()
{
    if (m_FontBaseName.length() != 0)
        return;

    PODOFO_ASSERT(m_FontName.length() != 0);
    m_SubsetPrefixLength = PoDoFo::GetSubsetPrefixLength(m_FontName);
    m_FontBaseName = PoDoFo::ExtractFontHints(string_view(m_FontName).substr(m_SubsetPrefixLength), m_IsItalicHint, m_IsBoldHint);
}

Corners PdfFontMetricsObject::getBBox(const PdfObject& obj)
{
    auto& arr = obj.GetArray();
    return Corners(
        arr[0].GetReal() * m_Matrix[0],
        arr[1].GetReal() * m_Matrix[3],
        arr[2].GetReal() * m_Matrix[0],
        arr[3].GetReal() * m_Matrix[3]
    );
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
