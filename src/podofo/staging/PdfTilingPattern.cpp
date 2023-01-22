/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfTilingPattern.h"

#include <iomanip>

#include <podofo/main/PdfDocument.h>
#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfColor.h>
#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfRect.h>
#include <podofo/main/PdfObjectStream.h>
#include <podofo/main/PdfWriter.h>
#include <podofo/main/PdfImage.h>
#include <podofo/main/PdfStringStream.h>
#include <podofo/main/PdfStreamDevice.h>

#include "PdfFunction.h"

using namespace std;
using namespace PoDoFo;

PdfTilingPattern::PdfTilingPattern(PdfDocument& doc, PdfTilingPatternType tilingType,
    double strokeR, double strokeG, double strokeB,
    bool doFill, double fillR, double fillG, double fillB,
    double offsetX, double offsetY,
    PdfImage* image)
    : PdfDictionaryElement(doc, "Pattern")
{
    PdfStringStream out;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ptrn" << this->GetObject().GetIndirectReference().ObjectNumber();

    m_Identifier = PdfName(out.GetString());

    this->Init(tilingType, strokeR, strokeG, strokeB,
        doFill, fillR, fillG, fillB, offsetX, offsetY, image);
}

void PdfTilingPattern::AddToResources(const PdfName& identifier, const PdfReference& ref, const PdfName& name)
{
    auto& resources = GetObject().GetDictionary().MustFindKey("Resources");
    if (!resources.GetDictionary().HasKey(name))
        resources.GetDictionary().AddKey(name, PdfDictionary());

    if (PdfDataType::Reference == resources.GetDictionary().GetKey(name)->GetDataType())
    {
        auto directObject = resources.GetDocument()->GetObjects().GetObject(resources.GetDictionary().GetKey(name)->GetReference());
        if (directObject == nullptr)
            PODOFO_RAISE_ERROR(PdfErrorCode::NoObject);

        if (!directObject->GetDictionary().HasKey(identifier))
            directObject->GetDictionary().AddKey(identifier, ref);
    }
    else
    {
        if (!resources.GetDictionary().GetKey(name)->GetDictionary().HasKey(identifier))
            resources.GetDictionary().GetKey(name)->GetDictionary().AddKey(identifier, ref);
    }
}

void PdfTilingPattern::Init(PdfTilingPatternType tilingType,
    double strokeR, double strokeG, double strokeB,
    bool doFill, double fillR, double fillG, double fillB,
    double offsetX, double offsetY,
    PdfImage* image)
{
    if (tilingType == PdfTilingPatternType::Image && image == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (tilingType != PdfTilingPatternType::Image && image != nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfRect rect;
    rect.SetLeft(0);
    rect.SetBottom(0);

    if (image != nullptr)
    {
        rect.SetWidth(image->GetWidth());
        rect.SetHeight(image->GetHeight()); // CHECK-ME: It was -image->GetHeight() but that appears to be wrong anyway
    }
    else
    {
        rect.SetWidth(8);
        rect.SetHeight(8);
    }

    PdfArray arr;
    rect.ToArray(arr);

    this->GetObject().GetDictionary().AddKey("PatternType", static_cast<int64_t>(1)); // Tiling pattern
    this->GetObject().GetDictionary().AddKey("PaintType", static_cast<int64_t>(1)); // Colored
    this->GetObject().GetDictionary().AddKey("TilingType", static_cast<int64_t>(1)); // Constant spacing
    this->GetObject().GetDictionary().AddKey("BBox", arr);
    this->GetObject().GetDictionary().AddKey("XStep", static_cast<int64_t>(rect.GetWidth()));
    this->GetObject().GetDictionary().AddKey("YStep", static_cast<int64_t>(rect.GetHeight()));
    this->GetObject().GetDictionary().AddKey("Resources", PdfDictionary());

    if (offsetX < -1e-9 || offsetX > 1e-9 || offsetY < -1e-9 || offsetY > 1e-9)
    {
        PdfArray arr;

        arr.Add(static_cast<int64_t>(1));
        arr.Add(static_cast<int64_t>(0));
        arr.Add(static_cast<int64_t>(0));
        arr.Add(static_cast<int64_t>(1));
        arr.Add(offsetX);
        arr.Add(offsetY);

        this->GetObject().GetDictionary().AddKey("Matrix", arr);
    }

    PdfStringStream out;

    if (image == nullptr)
    {
        if (doFill)
        {
            out << fillR << " " << fillG << " " << fillB << " rg" << " ";
            out << rect.GetLeft() << " " << rect.GetBottom() << " " << rect.GetWidth() << " " << rect.GetHeight() << " re" << " ";
            out << "f" << " "; //fill rect
        }

        out << strokeR << " " << strokeG << " " << strokeB << " RG" << " ";
        out << "2 J" << " "; // line capability style
        out << "0.5 w" << " "; //line width

        double left, bottom, right, top, whalf, hhalf;
        left = rect.GetLeft();
        bottom = rect.GetBottom();
        right = left + rect.GetWidth();
        top = bottom + rect.GetHeight();
        whalf = rect.GetWidth() / 2;
        hhalf = rect.GetHeight() / 2;

        switch (tilingType)
        {
            case PdfTilingPatternType::BDiagonal:
                out << left << " " << bottom << " m " << right << " " << top << " l ";
                out << left - whalf << " " << top - hhalf << " m " << left + whalf << " " << top + hhalf << " l ";
                out << right - whalf << " " << bottom - hhalf << " m " << right + whalf << " " << bottom + hhalf << " l" << std::endl;
                break;
            case PdfTilingPatternType::Cross:
                out << left << " " << bottom + hhalf << " m " << right << " " << bottom + hhalf << " l ";
                out << left + whalf << " " << bottom << " m " << left + whalf << " " << top << " l" << std::endl;
                break;
            case PdfTilingPatternType::DiagCross:
                out << left << " " << bottom << " m " << right << " " << top << " l ";
                out << left << " " << top << " m " << right << " " << bottom << " l" << std::endl;
                break;
            case PdfTilingPatternType::FDiagonal:
                out << left << " " << top << " m " << right << " " << bottom << " l ";
                out << left - whalf << " " << bottom + hhalf << " m " << left + whalf << " " << bottom - hhalf << " l ";
                out << right - whalf << " " << top + hhalf << " m " << right + whalf << " " << top - hhalf << " l" << std::endl;
                break;
            case PdfTilingPatternType::Horizontal:
                out << left << " " << bottom + hhalf << " m " << right << " " << bottom + hhalf << " l ";
                break;
            case PdfTilingPatternType::Vertical:
                out << left + whalf << " " << bottom << " m " << left + whalf << " " << top << " l" << std::endl;
                break;
            case PdfTilingPatternType::Image:
                /* This is handled above, based on the 'image' variable */
            default:
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
                break;

        }

        out << "S"; //stroke path
    }
    else
    {
        AddToResources(image->GetIdentifier(), image->GetObject().GetIndirectReference(), "XObject");

        out << rect.GetWidth() << " 0 0 "
            << rect.GetHeight() << " "
            << rect.GetLeft() << " "
            << rect.GetBottom() << " cm" << std::endl;
        out << "/" << image->GetIdentifier().GetString() << " Do" << std::endl;
    }

    GetObject().GetOrCreateStream().SetData(out.GetString(), { PdfFilterType::FlateDecode });
}
