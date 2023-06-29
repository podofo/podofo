/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfShadingPattern.h"

#include <podofo/main/PdfDocument.h>
#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfColor.h>
#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfObjectStream.h>
#include <podofo/main/PdfWriter.h>
#include <podofo/main/PdfStringStream.h>

#include "PdfFunction.h"


using namespace std;
using namespace PoDoFo;

PdfShadingPattern::PdfShadingPattern(PdfDocument& doc, PdfShadingPatternType shadingType)
    : PdfDictionaryElement(doc, "Pattern")
{
    PdfStringStream out;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Sh" << this->GetObject().GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName(out.GetString());

    this->Init(shadingType);
}

void PdfShadingPattern::Init(PdfShadingPatternType shadingType)
{
    /*
    switch( eShadingType )
    {
        case PdfShadingPatternType::FunctionBase:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case PdfShadingPatternType::Radial:
        {
            PdfArray coords;
            coords.push_back( 0.0 );
            coords.push_back( 0.0 );
            coords.push_back( 0.096 );
            coords.push_back( 0.0 );
            coords.push_back( 0.0 );
            coords.push_back( 1.0 );

            PdfArray domain;
            domain.push_back( 0.0 );
            domain.push_back( 1.0 );

            PdfArray c0;
            c0.push_back( 0.929 );
            c0.push_back( 0.357 );
            c0.push_back( 1.0 );
            c0.push_back( 0.298 );

            PdfArray c1a;
            c0.push_back( 0.631 );
            c0.push_back( 0.278 );
            c0.push_back( 1.0 );
            c0.push_back( 0.027 );

            PdfArray c1b;
            c0.push_back( 0.94 );
            c0.push_back( 0.4 );
            c0.push_back( 1.0 );
            c0.push_back( 0.102 );


            PdfExponentialFunction f1( domain, c0, c1a, 1.048, this->GetObject()->GetOwner() );
            PdfExponentialFunction f2( domain, c0, c1b, 1.374, this->GetObject()->GetOwner() );

            PdfFunction::List list;
            list.push_back( f1 );
            list.push_back( f2 );

            PdfArray bounds;
            bounds.push_back( 0.708 );

            PdfArray encode;
            encode.push_back( 1.0 );
            encode.push_back( 0.0 );
            encode.push_back( 0.0 );
            encode.push_back( 1.0 );

            PdfStitchingFunction function( list, domain, bounds, encode, this->GetObject()->GetOwner() );

            shading.AddKey( "Coords", coords );
            shading.AddKey( "Function", function.GetObject()->GetIndirectReference() );
            break;
        }
        case PdfShadingPatternType::FreeForm:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case PdfShadingPatternType::LatticeForm:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case PdfShadingPatternType::CoonsPatch:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case PdfShadingPatternType::TensorProduct:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        default:
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidEnumValue, "PdfShadingPattern::Init() failed because of an invalid shading pattern type");
        }
        };
    */

    // keys common to all shading directories
    PdfDictionary shading;
    shading.AddKey("ShadingType", static_cast<int64_t>(shadingType));

    this->GetObject().GetDictionary().AddKey("PatternType", static_cast<int64_t>(2)); // Shading pattern
    if (shadingType < PdfShadingPatternType::FreeForm)
    {
        this->GetObject().GetDictionary().AddKey("Shading", std::move(shading));
    }
    else
    {
        auto& shadingObject = this->GetObject().GetDocument()->GetObjects().CreateObject(std::move(shading));
        this->GetObject().GetDictionary().AddKey("Shading", shadingObject.GetIndirectReference());
    }
}

PdfAxialShadingPattern::PdfAxialShadingPattern(PdfDocument& doc, double x0, double y0, double x1, double y1, const PdfColor& start, const PdfColor& end)
    : PdfShadingPattern(doc, PdfShadingPatternType::Axial)
{
    Init(x0, y0, x1, y1, start, end);
}

void PdfAxialShadingPattern::Init(double x0, double y0, double x1, double y1, const PdfColor& start, const PdfColor& end)
{
    PdfArray coords;
    coords.Add(x0);
    coords.Add(y0);
    coords.Add(x1);
    coords.Add(y1);

    if (start.GetColorSpace() != end.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Colorspace of start and end color in PdfAxialShadingPattern does not match");

    PdfArray c0 = start.ToArray();
    PdfArray c1 = end.ToArray();
    PdfArray extend;

    extend.Add(true);
    extend.Add(true);

    PdfArray domain;
    domain.Add(0.0);
    domain.Add(1.0);

    PdfExponentialFunction function(*this->GetObject().GetDocument(), domain, c0, c1, 1.0);

    PdfDictionary& shading = this->GetObject().GetDictionary().GetKey(PdfName("Shading"))->GetDictionary();

    switch (start.GetColorSpace())
    {
        case PdfColorSpace::DeviceRGB:
            shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
            break;

        case PdfColorSpace::DeviceCMYK:
            shading.AddKey("ColorSpace", PdfName("DeviceCMYK"));
            break;

        case PdfColorSpace::DeviceGray:
            shading.AddKey("ColorSpace", PdfName("DeviceGray"));
            break;

        case PdfColorSpace::Lab:
        {
            PdfObject* csp = start.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Separation:
        {
            PdfObject* csp = start.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Colorspace not supported in PdfAxialShadingPattern");
            break;
    }

    shading.AddKey("Coords", coords);
    shading.AddKey("Function", function.GetObject().GetIndirectReference());
    shading.AddKey("Extend", extend);
}

PdfFunctionBaseShadingPattern::PdfFunctionBaseShadingPattern(PdfDocument& doc, const PdfColor& llCol, const PdfColor& ulCol, const PdfColor& lrCol, const PdfColor& urCol, const PdfArray& matrix)
    : PdfShadingPattern(doc, PdfShadingPatternType::FunctionBase)
{
    Init(llCol, ulCol, lrCol, urCol, matrix);
}

void PdfFunctionBaseShadingPattern::Init(const PdfColor& llCol, const PdfColor& ulCol, const PdfColor& lrCol, const PdfColor& urCol, const PdfArray& matrix)
{
    if (llCol.GetColorSpace() != ulCol.GetColorSpace() || ulCol.GetColorSpace() != lrCol.GetColorSpace() || lrCol.GetColorSpace() != urCol.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Colorspace of start and end color in PdfFunctionBaseShadingPattern does not match");

    PdfArray domain;
    domain.Add(0.0);
    domain.Add(1.0);
    domain.Add(0.0);
    domain.Add(1.0);

    PdfDictionary& shading = this->GetObject().GetDictionary().GetKey("Shading")->GetDictionary();
    PdfArray range;
    PdfSampledFunction::Sample samples;

    switch (llCol.GetColorSpace())
    {
        case PdfColorSpace::DeviceRGB:
        {
            range.Add(0.0);
            range.Add(1.0);
            range.Add(0.0);
            range.Add(1.0);
            range.Add(0.0);
            range.Add(1.0);

            samples.insert(samples.end(), static_cast<char> (llCol.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (llCol.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (llCol.GetBlue() * 255.0));

            samples.insert(samples.end(), static_cast<char> (lrCol.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetBlue() * 255.0));

            samples.insert(samples.end(), static_cast<char> (ulCol.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetBlue() * 255.0));

            samples.insert(samples.end(), static_cast<char> (urCol.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (urCol.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (urCol.GetBlue() * 255.0));

            shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
        }
        break;

        case PdfColorSpace::DeviceCMYK:
        {
            range.Add(0.0);
            range.Add(1.0);
            range.Add(0.0);
            range.Add(1.0);
            range.Add(0.0);
            range.Add(1.0);
            range.Add(0.0);
            range.Add(1.0);

            samples.insert(samples.end(), static_cast<char> (llCol.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (llCol.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (llCol.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (llCol.GetBlack() * 255.0));

            samples.insert(samples.end(), static_cast<char> (lrCol.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetBlack() * 255.0));

            samples.insert(samples.end(), static_cast<char> (ulCol.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetBlack() * 255.0));

            samples.insert(samples.end(), static_cast<char> (urCol.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (urCol.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (urCol.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (urCol.GetBlack() * 255.0));

            shading.AddKey("ColorSpace", PdfName("DeviceCMYK"));
        }
        break;

        case PdfColorSpace::DeviceGray:
        {
            range.Add(0.0);
            range.Add(1.0);

            samples.insert(samples.end(), static_cast<char> (llCol.GetGrayScale() * 255.0));

            samples.insert(samples.end(), static_cast<char> (lrCol.GetGrayScale() * 255.0));

            samples.insert(samples.end(), static_cast<char> (ulCol.GetGrayScale() * 255.0));

            samples.insert(samples.end(), static_cast<char> (urCol.GetGrayScale() * 255.0));

            shading.AddKey("ColorSpace", PdfName("DeviceGray"));
        }
        break;

        case PdfColorSpace::Lab:
        {
            range.Add(0.0);
            range.Add(100.0);
            range.Add(-128.0);
            range.Add(127.0);
            range.Add(-128.0);
            range.Add(127.0);

            samples.insert(samples.end(), static_cast<char> (llCol.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (llCol.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (llCol.GetCieB() + 128));

            samples.insert(samples.end(), static_cast<char> (lrCol.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetCieB() + 128));

            samples.insert(samples.end(), static_cast<char> (ulCol.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetCieB() + 128));

            samples.insert(samples.end(), static_cast<char> (urCol.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (urCol.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (urCol.GetCieB() + 128));

            PdfObject* csp = llCol.BuildColorSpace(*this->GetObject().GetDocument());

            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Separation:
        {
            range.Add(0.0);
            range.Add(1.0);

            samples.insert(samples.end(), static_cast<char> (llCol.GetDensity() * 255.0));
            samples.insert(samples.end(), static_cast<char> (lrCol.GetDensity() * 255.0));
            samples.insert(samples.end(), static_cast<char> (ulCol.GetDensity() * 255.0));
            samples.insert(samples.end(), static_cast<char> (urCol.GetDensity() * 255.0));

            PdfObject* csp = llCol.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Colorspace not supported in PdfFunctionBaseShadingPattern");
            break;
    }

    PdfSampledFunction function(*this->GetObject().GetDocument(), domain, range, samples);
    shading.AddKey("Function", function.GetObject().GetIndirectReference());
    shading.AddKey("Domain", domain);
    shading.AddKey("Matrix", matrix);
}

PdfRadialShadingPattern::PdfRadialShadingPattern(PdfDocument& doc, double x0, double y0, double r0, double x1, double y1, double r1, const PdfColor& start, const PdfColor& end)
    : PdfShadingPattern(doc, PdfShadingPatternType::Radial)
{
    Init(x0, y0, r0, x1, y1, r1, start, end);
}

void PdfRadialShadingPattern::Init(double x0, double y0, double r0, double x1, double y1, double r1, const PdfColor& start, const PdfColor& end)
{
    PdfArray coords;
    coords.Add(x0);
    coords.Add(y0);
    coords.Add(r0);
    coords.Add(x1);
    coords.Add(y1);
    coords.Add(r1);

    if (start.GetColorSpace() != end.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Colorspace of start and end color in PdfRadialShadingPattern does not match");

    PdfArray c0 = start.ToArray();
    PdfArray c1 = end.ToArray();
    PdfArray extend;

    extend.Add(true);
    extend.Add(true);

    PdfArray domain;
    domain.Add(0.0);
    domain.Add(1.0);

    PdfExponentialFunction function(*this->GetObject().GetDocument(), domain, c0, c1, 1.0);

    PdfDictionary& shading = this->GetObject().GetDictionary().GetKey("Shading")->GetDictionary();

    switch (start.GetColorSpace())
    {
        case PdfColorSpace::DeviceRGB:
            shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
            break;

        case PdfColorSpace::DeviceCMYK:
            shading.AddKey("ColorSpace", PdfName("DeviceCMYK"));
            break;

        case PdfColorSpace::DeviceGray:
            shading.AddKey("ColorSpace", PdfName("DeviceGray"));
            break;

        case PdfColorSpace::Lab:
        {
            PdfObject* csp = start.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Separation:
        {
            PdfObject* csp = start.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Colorspace not supported in PdfRadialShadingPattern");
            break;
    }

    shading.AddKey("Coords", coords);
    shading.AddKey("Function", function.GetObject().GetIndirectReference());
    shading.AddKey("Extend", extend);
}

PdfTriangleShadingPattern::PdfTriangleShadingPattern(PdfDocument& doc, double x0, double y0, const PdfColor& color0, double x1, double y1, const PdfColor& color1, double dX2, double dY2, const PdfColor& color2)
    : PdfShadingPattern(doc, PdfShadingPatternType::FreeForm)
{
    Init(x0, y0, color0, x1, y1, color1, dX2, dY2, color2);
}

void PdfTriangleShadingPattern::Init(double x0, double y0, const PdfColor& color0, double x1, double y1, const PdfColor& color1, double dX2, double dY2, const PdfColor& color2)
{
    if (color0.GetColorSpace() != color1.GetColorSpace() || color0.GetColorSpace() != color2.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Colorspace of start and end color in PdfTriangleShadingPattern does not match");

    PdfColor rgb0 = color0.ConvertToRGB();
    PdfColor rgb1 = color1.ConvertToRGB();
    PdfColor rgb2 = color2.ConvertToRGB();

    PdfArray decode;
    double minx, maxx, miny, maxy;

    minx = std::min(std::min(x0, x1), dX2);
    maxx = std::max(std::max(x0, x1), dX2);
    miny = std::min(std::min(y0, y1), dY2);
    maxy = std::max(std::max(y0, y1), dY2);

    decode.Add(minx);
    decode.Add(maxx);
    decode.Add(miny);
    decode.Add(maxy);

    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));

    PdfObject* shadingObject = this->GetObject().GetDictionary().FindKey("Shading");
    PdfDictionary& shading = shadingObject->GetDictionary();

    shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
    shading.AddKey("BitsPerCoordinate", static_cast<int64_t>(8));
    shading.AddKey("BitsPerComponent", static_cast<int64_t>(8));
    shading.AddKey("BitsPerFlag", static_cast<int64_t>(8));
    shading.AddKey("Decode", decode);

    // f x y c1 c2 c3
    int len = (1 + 1 + 1 + 1 + 1 + 1) * 3;
    char buff[18];

    buff[0] = 0; // flag - start new triangle
    buff[1] = static_cast<char>(255.0 * (x0 - minx) / (maxx - minx));
    buff[2] = static_cast<char>(255.0 * (y0 - miny) / (maxy - miny));
    buff[3] = static_cast<char>(255.0 * rgb0.GetRed());
    buff[4] = static_cast<char>(255.0 * rgb0.GetGreen());
    buff[5] = static_cast<char>(255.0 * rgb0.GetBlue());

    buff[6] = 0; // flag - start new triangle
    buff[7] = static_cast<char>(255.0 * (x1 - minx) / (maxx - minx));
    buff[8] = static_cast<char>(255.0 * (y1 - miny) / (maxy - miny));
    buff[9] = static_cast<char>(255.0 * rgb1.GetRed());
    buff[10] = static_cast<char>(255.0 * rgb1.GetGreen());
    buff[11] = static_cast<char>(255.0 * rgb1.GetBlue());

    buff[12] = 0; // flag - start new triangle
    buff[13] = static_cast<char>(255.0 * (dX2 - minx) / (maxx - minx));
    buff[14] = static_cast<char>(255.0 * (dY2 - miny) / (maxy - miny));
    buff[15] = static_cast<char>(255.0 * rgb2.GetRed());
    buff[16] = static_cast<char>(255.0 * rgb2.GetGreen());
    buff[17] = static_cast<char>(255.0 * rgb2.GetBlue());

    shadingObject->GetOrCreateStream().SetData(bufferview(buff, len));
}
