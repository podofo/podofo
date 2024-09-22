/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "colorchanger.h"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <iomanip>

#include "graphicsstack.h"
#include "iconverter.h"

using namespace std;
using namespace PoDoFo;

static const ColorChanger::KWInfo kwInfo[] = {
    { ColorChanger::eKeyword_GraphicsStack_Push,     "q", 0,    "Save state" },
    { ColorChanger::eKeyword_GraphicsStack_Pop,      "Q", 0,    "Restore state" },

    { ColorChanger::eKeyword_SelectGray_Stroking,    "G", 1,    "Select gray stroking color" },
    { ColorChanger::eKeyword_SelectRGB_Stroking,     "RG", 3,   "Select RGB stroking color" },
    { ColorChanger::eKeyword_SelectCMYK_Stroking,    "K", 4,    "Select CMYK stroking color" },

    { ColorChanger::eKeyword_SelectGray_NonStroking,    "g", 1,    "Select gray non-stroking color" },
    { ColorChanger::eKeyword_SelectRGB_NonStroking,     "rg", 3,   "Select RGB non-stroking color" },
    { ColorChanger::eKeyword_SelectCMYK_NonStroking,    "k", 4,    "Select CMYK non-stroking color" },

    { ColorChanger::eKeyword_SelectColorSpace_Stroking,    "CS", 1,    "Select colorspace non-stroking color" },
    { ColorChanger::eKeyword_SelectColorSpace_NonStroking,    "cs", 1,    "Select colorspace non-stroking color" },

    { ColorChanger::eKeyword_SelectColor_Stroking,    "SC", 1,    "Select depending on current colorspace" },
    { ColorChanger::eKeyword_SelectColor_NonStroking,    "sc", 1,    "Select depending on current colorspace" },
    { ColorChanger::eKeyword_SelectColor_Stroking2,    "SCN", 1,    "Select depending on current colorspace (extended)" },
    { ColorChanger::eKeyword_SelectColor_NonStroking2,    "scn", 1,    "Select depending on current colorspace (extended)" },

    // Sentinel
    { ColorChanger::eKeyword_Undefined,              "\0", 0,   NULL }
};


// PDF Commands, which modify colors according to PDFReference 1.7
// CS - select colorspace stroking (May need lookup in Colorspace key of resource directory)
// cs - select colorspace non-stroking (May need lookup in Colorspace key of resource directory)
// SC - select stroking color depending on colorspace
// SCN - select stroking color for colorspaces including Separation, DeviceN, ICCBased
// sc - select non-stroking color depending on colorspace
// scn - select non-stroking color for colorspaces including Separation, DeviceN, ICCBased
// G - select gray colorspace and gray stroking color
// g - select gray colorspace and gray non stroking color
// RG - select RGB colorspace and RGB stroking color
// rg - select RGB colorspace and RGB non stroking color
// K - select CMYK colorspace and CMYK stroking color
// k - select CMYK colorspace and CMYK non stroking color

// TODO: Allow to set default color and colorspace when starting a page

// ColorSpaces and their default colors
//  DeviceColorSpaces
//   DeviceGray 0.0
//   DeviceRGB 0.0
//   DeviceCMYK 0.0 0.0 0.0 1.0
//  CIE Based ColorSpaces
//   CalGray 0.0
//   CalRGB 0.0
//   Lab - all values 0.0 or closest according to range
//   ICCBased - all values 0.0 or closest according to range
//  Special ColorSpaces
//   Pattern - the value that causes nothing to be painted
//   Indexed 0
//   Separation - all values 1.0
//   DeviceN  - all values 1.0

// GraphicsState entries and their default values
//  ColorSpace - DeviceGray
//  color stroking - black (see ColorSpace default values)
//  color non stroking - black (see ColorSpace default values)
// Operations
//  q Push
//  Q Pop

ColorChanger::ColorChanger(IConverter* convert, const string_view& sInput, const string_view& sOutput)
    : m_converter(convert), m_input(sInput), m_output(sOutput)
{
    if (!m_converter)
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }
}

void ColorChanger::start()
{
    PdfMemDocument input;
    input.LoadFromBuffer(m_input);
    for (unsigned i = 0; i < input.GetPages().GetCount(); i++)
    {
        cout << "Processing page " << std::setw(6) << (i + 1) << "..." << endl << flush;

        auto& page = input.GetPages().GetPageAt(i);;

        m_converter->StartPage(page, i);
        this->ReplaceColorsInPage(page);
        m_converter->EndPage(page, i);
    }

    // Go through all XObjects
    PdfIndirectObjectList::iterator it = input.GetObjects().begin();
    while (it != input.GetObjects().end())
    {
        if ((*it)->IsDictionary() && (*it)->GetDictionary().HasKey("Type"))
        {
            if (PdfName("XObject") == (*it)->GetDictionary().GetKey("Type")->GetName()
                && (*it)->GetDictionary().HasKey("Subtype")
                && PdfName("Image") != (*it)->GetDictionary().GetKey("Subtype")->GetName())
            {
                cout << "Processing XObject " << (*it)->GetReference().ObjectNumber() << " "
                    << (*it)->GetReference().GenerationNumber() << endl;

                unique_ptr<PdfXObjectForm> xobj;
                (void)PdfXObject::TryCreateFromObject(**it, xobj);
                m_converter->StartXObject(*xobj);
                this->ReplaceColorsInPage(*xobj);
                m_converter->EndXObject(*xobj);
            }
        }
        it++;
    }

    input.Save(m_output);
}

void ColorChanger::ReplaceColorsInPage(PdfCanvas& page)
{
    PdfPostScriptTokenType t;
    string_view keyword;
    PdfVariant var;

    GraphicsStack graphicsStack;
    PdfPostScriptTokenizer tokenizer;
    vector<PdfVariant> args;

    PdfCanvasInputDevice input(page);

    charbuff buffer;
    BufferStreamDevice device(buffer);

    while (tokenizer.TryReadNext(input, t, keyword, var))
    {
        if (t == PdfPostScriptTokenType::Variant)
        {
            // arguments come before operators, but we want to group them up before
            // their operator.
            args.push_back(var);
        }
        ////else if (t == PdfPostScriptTokenType::ImageData)
        ////{
        ////    // Handle inline images (Internally using PdfData)
        ////    args.push_back(var);
        ////}
        else if (t == PdfPostScriptTokenType::Keyword)
        {
            const KWInfo* info = FindKeyWordByName(keyword.data());
            PdfColor color, newColor;
            int nNumArgs = info->nNumArguments;
            PdfColorSpaceType colorSpace;

            if (info->nNumArguments > 0 && args.size() != static_cast<size_t>(info->nNumArguments))
            {
                ostringstream oss;
                oss << "Expected " << info->nNumArguments << " argument(s) for keyword '" << keyword << "', but " << args.size() << " given instead.";
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidContentStream, oss.str().c_str());
            }

            switch (info->eKeywordType)
            {
                case eKeyword_GraphicsStack_Push:
                    graphicsStack.Push();
                    break;
                case eKeyword_GraphicsStack_Pop:
                    graphicsStack.Pop();
                    break;

                case eKeyword_SelectColorSpace_Stroking:
                    colorSpace = this->GetColorSpaceForName(args.back().GetName(), page);
                    colorSpace = PoDoFo::ConvertTo<PdfColorSpaceType>(args.back().GetName());
                    args.pop_back();
                    graphicsStack.SetStrokingColorSpace(colorSpace);
                    break;

                case eKeyword_SelectColorSpace_NonStroking:
                    colorSpace = PoDoFo::ConvertTo<PdfColorSpaceType>(args.back().GetName());
                    args.pop_back();
                    graphicsStack.SetNonStrokingColorSpace(colorSpace);
                    break;

                case eKeyword_SelectGray_Stroking:
                case eKeyword_SelectRGB_Stroking:
                case eKeyword_SelectCMYK_Stroking:
                case eKeyword_SelectGray_NonStroking:
                case eKeyword_SelectRGB_NonStroking:
                case eKeyword_SelectCMYK_NonStroking:

                    keyword =
                        this->ProcessColor(info->eKeywordType, nNumArgs, args, graphicsStack);

                    break;

                case eKeyword_SelectColor_Stroking:
                case eKeyword_SelectColor_Stroking2:
                {
                    /*
                    PdfError::LogMessage( LogSeverity::Information, "SCN called for colorspace: %s\n",
                                          PdfColor::GetNameForColorSpace(
                                              graphicsStack.GetStrokingColorSpace() ).GetName().c_str() );
                    */
                    int nTmpArgs;
                    EKeywordType eTempKeyword;

                    switch (graphicsStack.GetStrokingColorSpace())
                    {
                        case PdfColorSpaceType::DeviceGray:
                            nTmpArgs = 1;
                            eTempKeyword = eKeyword_SelectGray_Stroking;
                            break;
                        case PdfColorSpaceType::DeviceRGB:
                            nTmpArgs = 3;
                            eTempKeyword = eKeyword_SelectRGB_Stroking;
                            break;
                        case PdfColorSpaceType::DeviceCMYK:
                            nTmpArgs = 4;
                            eTempKeyword = eKeyword_SelectCMYK_Stroking;
                            break;

                        case PdfColorSpaceType::Separation:
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Error, "Separation color space not supported.\n");
                            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
                            break;
                        }
                        case PdfColorSpaceType::Lab:
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Error, "CieLab color space not supported.\n");
                            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
                            break;
                        }
                        case PdfColorSpaceType::Indexed:
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Error, "Indexed color space not supported.\n");
                            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
                            break;
                        }
                        case PdfColorSpaceType::Unknown:

                        default:
                        {
                            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
                        }
                    }

                    keyword =
                        this->ProcessColor(eTempKeyword, nTmpArgs, args, graphicsStack);
                    break;
                }

                case eKeyword_SelectColor_NonStroking:
                case eKeyword_SelectColor_NonStroking2:
                {
                    /*
                    PdfError::LogMessage( LogSeverity::Information,
                                          "scn called for colorspace: %s\n",
                                          PdfColor::GetNameForColorSpace(
                                          graphicsStack.GetNonStrokingColorSpace() ).GetName().c_str() );*/

                    int nTmpArgs;
                    EKeywordType eTempKeyword;

                    switch (graphicsStack.GetNonStrokingColorSpace())
                    {
                        case PdfColorSpaceType::DeviceGray:
                            nTmpArgs = 1;
                            eTempKeyword = eKeyword_SelectGray_NonStroking;
                            break;
                        case PdfColorSpaceType::DeviceRGB:
                            nTmpArgs = 3;
                            eTempKeyword = eKeyword_SelectRGB_NonStroking;
                            break;
                        case PdfColorSpaceType::DeviceCMYK:
                            nTmpArgs = 4;
                            eTempKeyword = eKeyword_SelectCMYK_NonStroking;
                            break;

                        case PdfColorSpaceType::Separation:
                        case PdfColorSpaceType::Lab:
                        case PdfColorSpaceType::Indexed:
                        case PdfColorSpaceType::Unknown:

                        default:
                        {
                            PoDoFo::LogMessage(PdfLogSeverity::Error, "Unknown color space {} type.\n",
                                PoDoFo::ToString(graphicsStack.GetNonStrokingColorSpace()));
                            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
                        }
                    }

                    keyword =
                        this->ProcessColor(eTempKeyword, nTmpArgs, args, graphicsStack);
                    break;
                }
                case eKeyword_Undefined:
                    //PdfError::LogMessage( LogSeverity::Error, "Unknown keyword type.\n" );
                    break;
                default:
                    break;
            }

            WriteArgumentsAndKeyword(args, keyword, device);
        }
    }

    // Write arguments if there are any left
    WriteArgumentsAndKeyword(args, { }, device);
    // Set new contents stream
    page.GetOrCreateContentsStream(PdfStreamAppendFlags::None).
        SetData(buffer);
}

void ColorChanger::WriteArgumentsAndKeyword(vector<PdfVariant>& args, const string_view& keyword, OutputStreamDevice& device)
{
    charbuff buffer;
    vector<PdfVariant>::const_iterator it = args.begin();
    while (it != args.end())
    {
        (*it).Write(device, PdfWriteFlags::None, { }, buffer);
        it++;
    }

    args.clear();

    if (!keyword.empty())
    {
        device.Write(" ", 1);
        device.Write(keyword);
        device.Write("\n", 1);
    }
}

const ColorChanger::KWInfo* ColorChanger::FindKeyWordByName(const string_view& keyword)
{
    PODOFO_RAISE_LOGIC_IF(keyword.empty(), "Keyword cannot be NULL.");

    const KWInfo* pInfo = &(kwInfo[0]);
    while (pInfo->eKeywordType != eKeyword_Undefined)
    {
        if (strcmp(pInfo->pszText, keyword.data()) == 0)
        {
            return pInfo;
        }

        pInfo++;
    }


    return pInfo;
}

void ColorChanger::PutColorOnStack(const PdfColor& rColor, vector<PdfVariant>& args)
{
    switch (rColor.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceGray:
            args.push_back(rColor.GetGrayScale());
            break;

        case PdfColorSpaceType::DeviceRGB:
            args.push_back(rColor.GetRed());
            args.push_back(rColor.GetGreen());
            args.push_back(rColor.GetBlue());
            break;

        case PdfColorSpaceType::DeviceCMYK:
            args.push_back(rColor.GetCyan());
            args.push_back(rColor.GetMagenta());
            args.push_back(rColor.GetYellow());
            args.push_back(rColor.GetBlack());
            break;

        case PdfColorSpaceType::Separation:
        case PdfColorSpaceType::Lab:
        case PdfColorSpaceType::Indexed:
        case PdfColorSpaceType::Unknown:

        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
        }
    }
}

PdfColor ColorChanger::GetColorFromStack(int nArgs, vector<PdfVariant>& args)
{
    PdfColor color;

    double gray = -1.0;
    double red = -1.0, green = -1.0, blue = -1.0;
    double cyan = -1.0, magenta = -1.0, yellow = -1.0, black = -1.0;
    switch (nArgs)
    {
        case 1:
            gray = args.back().GetReal();
            args.pop_back();
            color = PdfColor(gray);
            break;
        case 3:
            blue = args.back().GetReal();
            args.pop_back();
            green = args.back().GetReal();
            args.pop_back();
            red = args.back().GetReal();
            args.pop_back();
            color = PdfColor(red, green, blue);
            break;
        case 4:
            black = args.back().GetReal();
            args.pop_back();
            yellow = args.back().GetReal();
            args.pop_back();
            magenta = args.back().GetReal();
            args.pop_back();
            cyan = args.back().GetReal();
            args.pop_back();
            color = PdfColor(cyan, magenta, yellow, black);
            break;
    }

    return color;
}

const char* ColorChanger::ProcessColor(EKeywordType eKeywordType, int nNumArgs, vector<PdfVariant>& args, GraphicsStack& rGraphicsStack)
{
    PdfColor newColor;
    bool bStroking = false;
    PdfColor color = this->GetColorFromStack(nNumArgs, args);

    switch (eKeywordType)
    {

        case eKeyword_SelectGray_Stroking:
            bStroking = true;
            rGraphicsStack.SetStrokingColorSpace(PdfColorSpaceType::DeviceGray);
            newColor = m_converter->SetStrokingColorGray(color);
            break;

        case eKeyword_SelectRGB_Stroking:
            bStroking = true;
            rGraphicsStack.SetStrokingColorSpace(PdfColorSpaceType::DeviceRGB);
            newColor = m_converter->SetStrokingColorRGB(color);
            break;

        case eKeyword_SelectCMYK_Stroking:
            bStroking = true;
            rGraphicsStack.SetStrokingColorSpace(PdfColorSpaceType::DeviceCMYK);
            newColor = m_converter->SetStrokingColorCMYK(color);
            break;

        case eKeyword_SelectGray_NonStroking:
            rGraphicsStack.SetNonStrokingColorSpace(PdfColorSpaceType::DeviceGray);
            newColor = m_converter->SetNonStrokingColorGray(color);
            break;

        case eKeyword_SelectRGB_NonStroking:
            rGraphicsStack.SetNonStrokingColorSpace(PdfColorSpaceType::DeviceRGB);
            newColor = m_converter->SetNonStrokingColorRGB(color);
            break;

        case eKeyword_SelectCMYK_NonStroking:
            rGraphicsStack.SetNonStrokingColorSpace(PdfColorSpaceType::DeviceCMYK);
            newColor = m_converter->SetNonStrokingColorCMYK(color);
            break;

        case eKeyword_GraphicsStack_Push:
        case eKeyword_GraphicsStack_Pop:
        case eKeyword_SelectColorSpace_Stroking:
        case eKeyword_SelectColorSpace_NonStroking:
        case eKeyword_SelectColor_Stroking:
        case eKeyword_SelectColor_Stroking2:
        case eKeyword_SelectColor_NonStroking:
        case eKeyword_SelectColor_NonStroking2:
        case eKeyword_Undefined:

        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
        }
    }


    this->PutColorOnStack(newColor, args);
    if (bStroking)
    {
        rGraphicsStack.SetStrokingColor(newColor);
    }
    else
    {
        rGraphicsStack.SetNonStrokingColor(newColor);
    }

    return this->GetKeywordForColor(newColor, bStroking);
}

const char* ColorChanger::GetKeywordForColor(const PdfColor& rColor, bool bIsStroking)
{
    const char* pszKeyword = NULL;

    switch (rColor.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceGray:
            pszKeyword = (bIsStroking ? "G" : "g");
            break;

        case PdfColorSpaceType::DeviceRGB:
            pszKeyword = (bIsStroking ? "RG" : "rg");
            break;

        case PdfColorSpaceType::DeviceCMYK:
            pszKeyword = (bIsStroking ? "K" : "k");
            break;

        case PdfColorSpaceType::Separation:
        case PdfColorSpaceType::Lab:
        case PdfColorSpaceType::Indexed:
        case PdfColorSpaceType::Unknown:

        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
        }
    }

    return pszKeyword;
}

PdfColorSpaceType ColorChanger::GetColorSpaceForName(const PdfName& name, PdfCanvas& page)
{
    PdfColorSpaceType colorSpace = PoDoFo::ConvertTo<PdfColorSpaceType>(name);

    if (colorSpace == PdfColorSpaceType::Unknown)
    {
        // See if we can find it in the resource dictionary of the current page
        auto resources = page.GetResources();
        if (resources != NULL
            && resources->GetDictionary().HasKey("ColorSpace"))
        {
            auto colorSpaces = resources->GetDictionary().FindKey("ColorSpace");
            if (colorSpaces != NULL
                && colorSpaces->GetDictionary().HasKey(name))
            {
                auto cs = colorSpaces->GetDictionary().FindKey(name);
                if (!cs)
                {
                    PODOFO_RAISE_ERROR(PdfErrorCode::ObjectNotFound);
                }
                else if (cs->IsName())
                {
                    return this->GetColorSpaceForName(cs->GetName(), page);
                }
                else if (cs->IsArray())
                {
                    return this->GetColorSpaceForArray(cs->GetArray(), page);
                }
            }
        }
    }

    return colorSpace;
}

PdfColorSpaceType ColorChanger::GetColorSpaceForArray(const PdfArray&, PdfCanvas&)
{
    PdfColorSpaceType colorSpace = PdfColorSpaceType::Unknown;

    // CIE Based: [name dictionary]
    //     CalGray
    //     CalRGB
    //     CalLab
    //     ICCBased [name stream]
    // Special:
    //     Pattern
    //     Indexed [/Indexed base hival lookup]
    //     Separation [/Separation name alternateSpace tintTransform]
    //     DeviceN [/DeviceN names alternateSpace tintTransform] or
    //             [/DeviceN names alternateSpace tintTransform attributes]
    // 

    return colorSpace;
}
