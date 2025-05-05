#include <iostream>
#include <filesystem>

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/podofo.h>
#include <podofo/private/PdfFilterFactory.h>

namespace fs = std::filesystem;
using namespace std;
using namespace PoDoFo;

/**
 * Helper tool to build predefined CMap resources for PoDoFo
 * See ISO 32000-2:2020 "Table 116 — Predefined CJK CMap names"
 */

struct Context
{
    struct
    {
        unordered_set<string> Registries;
        unordered_set<string> Orderings;
        unordered_set<int> Supplements;
    } Catalog;
    vector<pair<string, string>> Maps;
};

static void handleCMapFolder(const fs::path& path, const unordered_set<string>& knowCmaps, Context& context, bool skipAddToContext);

static unique_ptr<FileStreamDevice> s_Stream;

int main()
{
    Context context;

    unordered_set<string> knowCmaps = {
        // Predefined CMaps
        "83pv-RKSJ-H",
        "90ms-RKSJ-H",
        "90ms-RKSJ-V",
        "90msp-RKSJ-H",
        "90msp-RKSJ-V",
        "90pv-RKSJ-H",
        "Add-RKSJ-H",
        "Add-RKSJ-V",
        "B5pc-H",
        "B5pc-V",
        "CNS-EUC-H",
        "CNS-EUC-V",
        "ETen-B5-H",
        "ETen-B5-V",
        "ETenms-B5-H",
        "ETenms-B5-V",
        "EUC-H",
        "EUC-V",
        "Ext-RKSJ-H",
        "Ext-RKSJ-V",
        "GB-EUC-H",
        "GB-EUC-V",
        "GBK-EUC-H",
        "GBK-EUC-V",
        "GBK2K-H",
        "GBK2K-V",
        "GBKp-EUC-H",
        "GBKp-EUC-V",
        "GBpc-EUC-H",
        "GBpc-EUC-V",
        "H",
        "HKscs-B5-H",
        "HKscs-B5-V",
        "KSC-EUC-H",
        "KSC-EUC-V",
        "KSCms-UHC-H",
        "KSCms-UHC-HW-H",
        "KSCms-UHC-HW-V",
        "KSCms-UHC-V",
        "KSCpc-EUC-H",
        "UniCNS-UCS2-H",
        "UniCNS-UCS2-V",
        "UniCNS-UTF16-H",
        "UniCNS-UTF16-V",
        "UniGB-UCS2-H",
        "UniGB-UCS2-V",
        "UniGB-UTF16-H",
        "UniGB-UTF16-V",
        "UniJIS-UCS2-H",
        "UniJIS-UCS2-HW-H",
        "UniJIS-UCS2-HW-V",
        "UniJIS-UCS2-V",
        "UniJIS-UTF16-H",
        "UniJIS-UTF16-V",
        "UniKS-UCS2-H",
        "UniKS-UCS2-V",
        "UniKS-UTF16-H",
        "UniKS-UTF16-V",
        "V",
        // ToUnicode maps
        "Adobe-CNS1-UCS2",
        "Adobe-GB1-UCS2",
        "Adobe-Japan1-UCS2",
        "Adobe-Korea1-UCS2",
    };

    auto currentPath = fs::current_path();
    auto mainPodofoSrcPath = currentPath.parent_path().parent_path().parent_path().parent_path().parent_path()
        / "src" / "podofo" / "main";

    // Find a repository that has both "cmap-resources" (https://github.com/adobe-type-tools/cmap-resources)
    // and "mapping-resources-pdf" (https://github.com/adobe-type-tools/mapping-resources-pdf)
    auto adobeTypeToolsRepoPath = utls::GetEnvironmentVariable("ADOBE_TYPE_TOOLS_REPOSITORY");
    if (adobeTypeToolsRepoPath.empty())
        throw runtime_error("ADOBE_TYPE_TOOLS_REPOSITORY environment variable is missing");

    s_Stream.reset(new FileStreamDevice((mainPodofoSrcPath / "PdfEncodingMapFactory_PredefinedCMaps.cpp").u8string(), FileMode::Create));
    s_Stream->Write(R"(// This file was generated. DO NOT EDIT!
/**
 * SPDX-FileCopyrightText: (C) 2024 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */
#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingMapFactory.h"
#include <podofo/private/PdfFilterFactory.h>

using namespace std;
using namespace PoDoFo;

static void buildMappings(const string_view& serialized, CodeUnitMap& mappings, CodeUnitRanges& ranges);

namespace
{
    using MapGetter = std::add_pointer<const PdfCMapEncodingConstPtr&()>::type;
}
)");

    s_Stream->Write("\nnamespace PoDoFo\n");
    s_Stream->Write("{\n");
    s_Stream->Write("    class PdfCMapEncodingFactory\n");
    s_Stream->Write("    {\n");
    s_Stream->Write("    public:\n");
    handleCMapFolder(fs::u8path(adobeTypeToolsRepoPath) / "cmap-resources", knowCmaps, context, true);
    handleCMapFolder(fs::u8path(adobeTypeToolsRepoPath) / "mapping-resources-pdf", knowCmaps, context, false);
    s_Stream->Write("    };\n");
    s_Stream->Write("}\n");
    s_Stream->Write("\nstatic unordered_map<string_view, MapGetter> s_PredefinedCMaps = {\n");
    for (auto& map : context.Maps)
    {
        s_Stream->Write("    { \"");
        s_Stream->Write(map.first);
        s_Stream->Write("\", &PdfCMapEncodingFactory::");
        s_Stream->Write(map.second);
        s_Stream->Write(" },\n");
    }
    s_Stream->Write("};\n");
    s_Stream->Write(R"(
PdfCMapEncodingConstPtr PdfEncodingMapFactory::GetPredefinedCMap(const string_view& cmapName)
{
    auto found = s_PredefinedCMaps.find(cmapName);
    if (found == s_PredefinedCMaps.end())
        return nullptr;
    else
        return found->second();
}

const PdfCMapEncoding& PdfEncodingMapFactory::GetPredefinedCMapInstance(const string_view& cmapName)
{
    auto found = s_PredefinedCMaps.find(cmapName);
    if (found == s_PredefinedCMaps.end())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncoding, "Could not find a cmap with a CMap name {}", cmapName);

    return *found->second();
}

unsigned readCode(InputStream& stream, unsigned char codeSize)
{
    switch (codeSize)
    {
        case 1:
        {
            return (unsigned char)stream.ReadChar();
        }
        case 2:
        {
            uint16_t code;
            utls::ReadUInt16BE(stream, code);
            return code;
        }
        case 3:
        {
            uint24_t code;
            utls::ReadUInt24BE(stream, code);
            return code;
        }
        case 4:
        {
            uint32_t code;
            utls::ReadUInt32BE(stream, code);
            return code;
        }
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }
}

void readMapping(InputStream& stream, CodeUnitMap& mappings, vector<codepoint>& temp)
{
    unsigned char codeSize = (unsigned char)stream.ReadChar();
    unsigned code = readCode(stream, codeSize);
    unsigned char copdePointsSize = (unsigned char)stream.ReadChar();
    temp.resize(copdePointsSize);
    uint32_t cp;
    for (unsigned char i = 0; i < copdePointsSize; i++)
    {
        utls::ReadUInt32BE(stream, cp);
        temp[i] = cp;
    }

    mappings[PdfCharCode(code, codeSize)] = CodePointSpan(temp);
}

void readRange(InputStream& stream, CodeUnitRanges& ranges, vector<codepoint>& temp)
{
    unsigned char codeSize = (unsigned char)stream.ReadChar();
    unsigned code = readCode(stream, codeSize);

    uint16_t rangeSize;
    utls::ReadUInt16BE(stream, rangeSize);

    unsigned char copdePointsSize = (unsigned char)stream.ReadChar();
    temp.resize(copdePointsSize);
    uint32_t cp;
    for (unsigned char i = 0; i < copdePointsSize; i++)
    {
        utls::ReadUInt32BE(stream, cp);
        temp[i] = cp;
    }

    ranges.insert(CodeUnitRange{ PdfCharCode(code, codeSize), rangeSize, CodePointSpan(temp) });
}

void buildMappings(const string_view& compressed, CodeUnitMap& mappings, CodeUnitRanges& ranges)
{
    auto filter = PdfFilterFactory::Create(PdfFilterType::FlateDecode);
    charbuff serialized;
    filter->DecodeTo(serialized, compressed);

    SpanStreamDevice stream(serialized);
    uint16_t size;
    utls::ReadUInt16BE(stream, size);
    mappings.reserve(size);
    vector<codepoint> temp;
    for (unsigned i = 0; i < size; i++)
        readMapping(stream, mappings, temp);

    utls::ReadUInt16BE(stream, size);
    for (unsigned i = 0; i < size; i++)
        readRange(stream, ranges, temp);
}
)");

    cout << "Registries: [" << endl;
    for (auto& registry : context.Catalog.Registries)
        cout << "   " << registry << "," << endl;
    cout << "]" << endl;
    cout << "Orderings:" << endl;
    for (auto& ordering : context.Catalog.Orderings)
        cout << "   " << ordering << "," << endl;
    cout << "]" << endl;
    cout << "Supplements:" << endl;
    for (auto& supplement : context.Catalog.Supplements)
        cout << "   " << supplement << "," << endl;
    cout << "]" << endl;
    return 0;
}

void serializeCode(OutputStream& stream, unsigned code, unsigned char codeSize)
{
    switch (codeSize)
    {
        case 1:
            stream.Write((char)(uint8_t)code);
            break;
        case 2:
            utls::WriteUInt16BE(stream, (uint16_t)code);
            break;
        case 3:
            utls::WriteUInt24BE(stream, (uint24_t)code);
            break;
        case 4:
            utls::WriteUInt32BE(stream, code);
            break;
        default:
            throw runtime_error("Invalid code size");
    }
}

void writeString(const string_view& view)
{
    const int max_line_length = 16;
    std::string line;
    line.push_back('"');

    for (size_t i = 0; i < view.size(); i++)
    {
        // Convert character to octal and append to the line
        line += fmt::format("\\{:03o}", static_cast<unsigned char>(view[i]));

        // If the line reaches the maximum length or it's the last character, write to the output
        if ((i + 1) % max_line_length == 0 || i == view.size() - 1)
        {
            line.append("\"\n");
            s_Stream->Write(line);
            line.clear();
            line.push_back('"');
        }
    }
}

void write(const PdfCharCodeMap& map)
{
    string serialized;
    ContainerStreamDevice stream(serialized);

    auto& mappings = map.GetMappings();
    assert(mappings.size() <= 65535);
    utls::WriteUInt16BE(stream, (uint16_t)mappings.size());
    for (auto& pair : mappings)
    {
        stream.Write((char)pair.first.CodeSpaceSize);
        serializeCode(stream, pair.first.Code, pair.first.CodeSpaceSize);
        assert(pair.second.size() <= 255);
        stream.Write((char)(uint8_t)pair.second.size());
        for (unsigned i = 0; i < pair.second.size(); i++)
            utls::WriteUInt32BE(stream, pair.second[i]);
    }

    auto& ranges = map.GetRanges();
    assert(ranges.size() <= 65535);
    utls::WriteUInt16BE(stream, (uint16_t)ranges.size());
    for (auto& range : ranges)
    {
        stream.Write((char)range.SrcCodeLo.CodeSpaceSize);
        serializeCode(stream, range.SrcCodeLo.Code, range.SrcCodeLo.CodeSpaceSize);
        assert(range.Size <= 65535);
        utls::WriteUInt16BE(stream, (uint16_t)range.Size);
        assert(range.DstCodeLo.size() <= 255);
        stream.Write((char)(uint8_t)range.DstCodeLo.size());
        for (unsigned i = 0; i < range.DstCodeLo.size(); i++)
            utls::WriteUInt32BE(stream, range.DstCodeLo[i]);
    }

    auto filter = PdfFilterFactory::Create(PdfFilterType::FlateDecode);
    charbuff compressed;
    filter->EncodeTo(compressed, serialized);
    writeString(compressed);
}

void write(const PdfCMapEncoding& encoding, Context& context)
{
    s_Stream->Write("        static const PdfCMapEncodingConstPtr& ");
    auto& info = encoding.GetCIDSystemInfo();
    auto& map = encoding.GetCharMap();
    auto& limits = encoding.GetLimits();
    auto& mapLimits = map.GetLimits();
    auto name = encoding.GetName().GetString();
    string methodName = "Get_";
    methodName.append(name);
    std::replace(methodName.begin(), methodName.end(), '-', '_');
    s_Stream->Write(methodName);
    s_Stream->Write("()\n");
    s_Stream->Write("        {\n");
    s_Stream->Write("            static constexpr const char serialized[] =\n");
    write(map);
    s_Stream->Write(";\n");
    s_Stream->Write("            static struct Init\n");
    s_Stream->Write("            {\n");
    s_Stream->Write("                Init()\n");
    s_Stream->Write("                {\n");
    s_Stream->Write("                    CodeUnitMap mappings;\n");
    s_Stream->Write("                    CodeUnitRanges ranges;\n");
    s_Stream->Write("                    buildMappings(string_view(serialized, std::size(serialized) - 1), mappings, ranges);\n");
    s_Stream->Write("                    map.reset(new PdfCMapEncoding(PdfCharCodeMap(\n");
    s_Stream->Write(fmt::format("                        std::move(mappings), std::move(ranges), PdfEncodingLimits({}, {}, PdfCharCode({}, {}), PdfCharCode({}, {}))),\n",
        mapLimits.MinCodeSize, mapLimits.MaxCodeSize, mapLimits.FirstChar.Code, mapLimits.FirstChar.CodeSpaceSize, mapLimits.LastChar.Code, mapLimits.LastChar.CodeSpaceSize));
    s_Stream->Write(fmt::format(R"(                        true, "{}"_n, PdfCIDSystemInfo{{ "{}", "{}", {} }}, {}, PdfEncodingLimits({}, {}, PdfCharCode({}, {}), PdfCharCode({}, {}))));)",
        name, info.Registry.GetString(), info.Ordering.GetString(), info.Supplement, (encoding.GetWMode() == PdfWModeKind::Vertical ? 1 : 0),
        limits.MinCodeSize, limits.MaxCodeSize, limits.FirstChar.Code, limits.FirstChar.CodeSpaceSize, limits.LastChar.Code, limits.LastChar.CodeSpaceSize));
    s_Stream->Write("\n                }\n");
    s_Stream->Write("                PdfCMapEncodingConstPtr map;\n");
    s_Stream->Write("            } init;\n");
    s_Stream->Write("            return init.map;\n");
    s_Stream->Write("        }\n\n");
    context.Maps.push_back({ (string)name, methodName });
}

void handleCMapFolder(const fs::path& path, const unordered_set<string>& knowCmaps, Context& context, bool addToCatalog)
{
    // Iterate through the directory entries
    for (const auto& outerFolder : fs::directory_iterator(path))
    {
        if (outerFolder.path().u8string().find(".git", 0) != string::npos)
            continue;

        if (outerFolder.path().u8string().find("deprecated", 0) != string::npos)
            continue;

        if (fs::is_directory(outerFolder.status()))
        {
            auto cmapfolder = outerFolder.path() / "CMap";
            const fs::path* actualPath;
            if (fs::exists(cmapfolder))
                actualPath = &cmapfolder;
            else
                actualPath = &outerFolder.path();

            for (auto& entry : fs::directory_iterator(*actualPath))
            {
                auto& entryPath = entry.path();
                if (knowCmaps.find(entryPath.filename().u8string()) == knowCmaps.end())
                    continue;

                cout << entryPath.u8string() << endl;
                auto cmap = PdfCMapEncoding::Parse(entryPath.u8string());
                cout << "Name: " << cmap.GetName().GetString()
                    << ", Ordering: " << cmap.GetCIDSystemInfo().Registry.GetString()
                    << ", Ordering: " << cmap.GetCIDSystemInfo().Ordering.GetString()
                    << ", Supplement: " << cmap.GetCIDSystemInfo().Supplement
                    << ", WMode: " << (cmap.GetWMode() == PdfWModeKind::Horizontal ? "H" : "V") << endl;

                write(cmap, context);

                if (addToCatalog)
                {
                    context.Catalog.Registries.insert((string)cmap.GetCIDSystemInfo().Registry);
                    context.Catalog.Orderings.insert((string)cmap.GetCIDSystemInfo().Ordering);
                    context.Catalog.Supplements.insert(cmap.GetCIDSystemInfo().Supplement);
                }
            }
        }
    }
}
