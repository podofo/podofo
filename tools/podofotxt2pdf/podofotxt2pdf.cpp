/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/podofo.h>

#include <cstdlib>
#include <cstdio>

// Conversation constant to convert 1/1000th mm to 1/72 inch
#define CONVERSION_CONSTANT 0.002834645669291339

using namespace std;
using namespace PoDoFo;

#define BORDER_TOP 10000 * CONVERSION_CONSTANT
#define BORDER_LEFT 10000 * CONVERSION_CONSTANT
#define FONT_SIZE 12.0
#define DEFAULT_FONT "Arial"

void print_help()
{
    printf("Usage: podofotxt2pdf [inputfile] [outputfile]\n\n");
    printf("Optional parameters:\n");
    printf("\t-fontname [name]\t Use the font [name]\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void draw(const char* buffer, PdfDocument& document, const string_view& fontName)
{
    PdfPainter painter;
    PdfRect size;

    double x = BORDER_LEFT;
    double y = BORDER_TOP;
    const char* cursor = buffer;

    size = PdfPage::CreateStandardPageSize(PdfPageSize::A4);
    auto font = document.GetFonts().SearchFont(fontName);
    auto page = &document.GetPages().CreatePage(size);
    y = size.GetHeight() - y;
    if (font == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    painter.SetCanvas(*page);
    painter.GetTextState().SetFont(*font, FONT_SIZE);

    while (*buffer)
    {
        if (*buffer == '\n')
        {
            painter.DrawText(cursor, x, y);
            cursor = ++buffer;
            y -= font->GetMetrics().GetLineSpacing();
            if (y < BORDER_TOP)
            {
                page = &document.GetPages().CreatePage(size);
                painter.SetCanvas(*page);
                y = size.GetHeight() - y;
            }
        }
        else
        {
            buffer++;
        }
    }

    painter.FinishDrawing();
}

void init(const string_view& inputPath, const string_view& outputPath, const string_view& fontName)
{
    FILE* file;

    PdfStreamedDocument doc(outputPath);

    char* buffer;
    long size;

    file = fopen(inputPath.data(), "rb");	// read it as binary if we are going to compare sizes!
    if (!file)
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    if (fseek(file, 0x00, SEEK_END) == -1)
    {
        fclose(file);
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to seek to the end of the file");
    }

    size = ftell(file);
    if (size == -1)
    {
        fclose(file);
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to read size of the file");
    }

    buffer = static_cast<char*>(malloc(sizeof(char) * (size + 1)));
    fseek(file, 0x00, SEEK_SET);
    if (!buffer)
    {
        fclose(file);
        PODOFO_RAISE_ERROR(PdfErrorCode::OutOfMemory);
    }

    // read the whole file into memory at once.
    // this not very efficient, but as this is 
    // a library demonstration I do not care.
    // If anyone wants to improve this: Go for it!
    if (static_cast<long>(fread(buffer, sizeof(char), size, file)) != size)
    {
        free(buffer);
        fclose(file);
        PODOFO_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);
    }

    fclose(file);

    buffer[size] = '\0';

    draw(buffer, doc, fontName);

    doc.GetMetadata().SetCreator(PdfString("podofotxt2pdf"));
    doc.GetMetadata().SetTitle(PdfString("Converted to PDF from a text file"));
    doc.Close();

    free(buffer);
}

int main(int argc, char* argv[])
{
    const char* inputPath = NULL;
    const char* outputPath = NULL;
    const char* fontName = DEFAULT_FONT;

    if (argc < 3)
    {
        print_help();
        exit(-1);
    }

    for (int i = 1; i < argc; i++)
    {

        if (strcmp("-fontname", argv[i]) == 0)
        {
            fontName = argv[++i];
        }
        else
        {
            if (inputPath == NULL)
                inputPath = argv[i];
            else
                outputPath = argv[i];
        }
    }

    try
    {
        init(inputPath, outputPath, fontName);
    }
    catch (PdfError& e)
    {
        fprintf(stderr, "Error %i occurred!\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    return 0;
}
