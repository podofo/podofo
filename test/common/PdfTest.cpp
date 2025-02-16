/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfTest.h"

#include <utf8cpp/utf8.h>

using namespace std;
using namespace PoDoFo;

static void readTestInputFile(const string_view& filepath, string& str);
static void writeTestOutputFile(const string_view& filepath, const string_view& view);

static struct TestPaths
{
    TestPaths() :
        Input(PDF_TEST_RESOURCE_PATH), Output(PDF_TEST_OUTPUT_PATH) { }
    fs::path Input;
    fs::path Output;
} s_paths;


void utls::CombinePaths(std::filesystem::path& path, std::initializer_list<std::string_view> paths)
{
    for (auto& entry : paths)
        path /= std::filesystem::u8path(entry);
}

string TestUtils::GetTestOutputFilePath(const string_view& filename)
{
    return (s_paths.Output / fs::u8path(filename)).u8string();
}

string TestUtils::GetTestInputFilePath(const string_view& filename)
{
    return (s_paths.Input / fs::u8path(filename)).u8string();
}

const fs::path& TestUtils::GetTestInputPath()
{
    return s_paths.Input;
}

const fs::path& TestUtils::GetTestOutputPath()
{
    return s_paths.Output;
}

void TestUtils::ReadTestInputFile(const string_view& filename, string& str)
{
    readTestInputFile(GetTestInputFilePath(filename), str);
}

void TestUtils::WriteTestOutputFile(const string_view& filename, const string_view& view)
{
    writeTestOutputFile(GetTestOutputFilePath(filename), view);
}

void TestUtils::AssertEqual(double expected, double actual, double threshold)
{
    if (std::abs(actual - expected) > threshold)
        FAIL("Expected different than actual");
}

void TestUtils::SaveFramePPM(charbuff& buffer, const void* data,
    PdfPixelFormat srcPixelFormat, unsigned width, unsigned height)
{
    BufferStreamDevice stream(buffer);
    SaveFramePPM(stream, data, srcPixelFormat, width, height);
}

void TestUtils::SaveFramePPM(OutputStream& stream, const void* data,
    PdfPixelFormat srcPixelFormat, unsigned width, unsigned height)
{
    // Write header
    stringstream headerStream;
    headerStream << "P7\nWIDTH " << width << "\nHEIGHT " << height << "\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";

    stream.Write(headerStream.str());

    unsigned lineSize = width * 4;
    charbuff scanline(lineSize);

    // Write pixel data
    switch (srcPixelFormat)
    {
        case PdfPixelFormat::RGBA:
        {
            stream.Write((const char*)data, (size_t)width * height * 4ULL);
            break;
        }
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i1 = 0; i1 < height; i1++)
            {
                for (unsigned i2 = 0; i2 < width; i2++)
                {
                    scanline[i2 * 4 + 0] = ((const unsigned char*)data)[i1 * lineSize + i2 * 4 + 2];
                    scanline[i2 * 4 + 1] = ((const unsigned char*)data)[i1 * lineSize + i2 * 4 + 1];
                    scanline[i2 * 4 + 2] = ((const unsigned char*)data)[i1 * lineSize + i2 * 4 + 0];
                    scanline[i2 * 4 + 3] = ((const unsigned char*)data)[i1 * lineSize + i2 * 4 + 3];
                }

                stream.Write(scanline.data(), scanline.size());
            }
            break;
        }
        case PdfPixelFormat::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    stream.Flush();
}

bool TestUtils::IsBufferEqual(const bufferview& buffer, const string_view& filename)
{
    charbuff tmp;
    utls::ReadTo(tmp, filename);
    return buffer == tmp;
}

bool TestUtils::AreFilesEqual(const string_view& filename1, const string_view& filename2)
{
    charbuff tmp1;
    utls::ReadTo(tmp1, filename1);

    charbuff tmp2;
    utls::ReadTo(tmp2, filename2);
    return tmp1 == tmp2;
}

void readTestInputFile(const string_view& filepath, string& str)
{
#ifdef _WIN32
    auto filepath16 = utf8::utf8to16((string)filepath);
    ifstream stream((wchar_t*)filepath16.c_str(), ios_base::binary);
#else
    ifstream stream((string)filepath, ios_base::binary);
#endif

    stream.seekg(0, ios::end);
    if (stream.tellg() == -1)
        throw runtime_error("Error reading from stream");

    str.reserve((size_t)stream.tellg());
    stream.seekg(0, ios::beg);

    str.assign((istreambuf_iterator<char>(stream)), istreambuf_iterator<char>());
}

void writeTestOutputFile(const string_view& filepath, const string_view& view)
{
#ifdef _WIN32
    auto filepath16 = utf8::utf8to16((string)filepath);
    ofstream stream((wchar_t*)filepath16.c_str(), ios_base::binary);
#else
    ofstream stream((string)filepath, ios_base::binary);
#endif

    stream.write(view.data(), view.size());
    stream.close();
    if (stream.fail())
        throw runtime_error("Error writing to stream");
}
