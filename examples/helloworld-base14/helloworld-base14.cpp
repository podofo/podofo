/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// Include the standard headers for cout to write
// some output to the console.
#include <iostream>

// Now include all PoDoFo header files, to have access
// to all functions of PoDoFo and so that you do not have
// to care about the order of includes.
// 
// You should always use podofo.h and not try to include
// the required headers on your own.
#include <podofo/podofo.h>

// All PoDoFo classes are member of the PoDoFo namespace.
using namespace std;
using namespace PoDoFo;

void PrintHelp()
{
    std::cout << "This is a example application for the PoDoFo PDF library." << std::endl
        << "It creates a small PDF file containing the text >Hello World!<" << std::endl
        << "Please see https://github.com/podofo/podofo for more information" << std::endl << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  helloworld-base14 [outputfile.pdf]" << std::endl << std::endl;
}

const char* GetBase14FontName(unsigned i);
void DemoBase14Fonts(PdfPainter& painter, PdfPage& page, PdfDocument& document, const PdfFontSearchParams& params);

void HelloWorld(const string_view& filename)
{
    PdfMemDocument document;

    // PdfPainter is the class which is able to draw text and graphics
    // directly on a PdfPage object.
    PdfPainter painter;

    // A PdfFont object is required to draw text on a PdfPage using a PdfPainter.
    // PoDoFo will find the font using fontconfig on your system and embedd truetype
    // fonts automatically in the PDF file.
    PdfFont* font;

    try
    {
        // The PdfDocument object can be used to create new PdfPage objects.
        // The PdfPage object is owned by the PdfDocument will also be deleted automatically
        // by the PdfDocument object.
        // 
        // You have to pass only one argument, i.e. the page size of the page to create.
        // There are predefined enums for some common page sizes.
        auto& page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));

        // If the page cannot be created because of an error (e.g. ePdfError_OutOfMemory )
        // a nullptr pointer is returned.
        // We check for a nullptr pointer here and throw an exception using the RAISE_ERROR macro.
        // The raise error macro initializes a PdfError object with a given error code and
        // the location in the file in which the error ocurred and throws it as an exception.

        // Set the page as drawing target for the PdfPainter.
        // Before the painter can draw, a page has to be set first.
        painter.SetCanvas(page);

        // Create a PdfFont object using the font "Arial".
        // The font is found on the system using fontconfig and embedded into the
        // PDF file. If Arial is not available, a default font will be used.
        // 
        // The created PdfFont will be deleted by the PdfDocument.
        PdfFontSearchParams params;
        params.AutoSelect = PdfFontAutoSelectBehavior::Standard14;
        font = document.GetFonts().SearchFont("Helvetica", params);

        // If the PdfFont object cannot be allocated return an error.
        if (font == nullptr)
            throw runtime_error("Invalid handle");

        // Set the font as default font for drawing.
        // A font has to be set before you can draw text on
        // a PdfPainter.
        painter.GetTextState().SetFont(*font, 18.0);

        // You could set a different color than black to draw
        // the text.
        // 
        // painter.SetColor(1.0, 0.0, 0.0);

        // Actually draw the line "Hello World!" on to the PdfPage at
        // the position 2cm,2cm from the top left corner.
        // Please remember that PDF files have their origin at the
        // bottom left corner. Therefore we substract the y coordinate
        // from the page height.
        //
        // The position specifies the start of the baseline of the text.
        //
        // All coordinates in PoDoFo are in PDF units.
        painter.DrawText("Hello World!", 56.69, page.GetRect().GetHeight() - 56.69);

        DemoBase14Fonts(painter, page, document, params);

        painter.FinishDrawing();

        // The last step is to close the document.
        document.Save(filename);

    }
    catch (PdfError& e)
    {
        // All PoDoFo methods may throw exceptions
        // make sure that painter.FinishPage() is called
        // or who will get an assert in its destructor
        try
        {
            painter.FinishDrawing();
        }
        catch (...)
        {
            // Ignore errors this time
        }

        throw e;
    }
}

int main(int argc, char* argv[])
{
    // Check if a filename was passed as commandline argument.
    // If more than 1 argument or no argument is passed,
    // a help message is displayed and the example application
    // will quit.
    if (argc != 2)
    {
        PrintHelp();
        return -1;
    }

    // All PoDoFo functions will throw an exception in case of an error.
    // 
    // You should catch the exception to either fix it or report
    // back to the user.
    // 
    // All exceptions PoDoFo throws are objects of the class PdfError.
    // Thats why we simply catch PdfError objects.
    try
    {
        // Call the drawing routing which will create a PDF file
        // with the filename of the output file as argument.
        HelloWorld(argv[1]);
    }
    catch (PdfError& err)
    {
        // We have to check if an error has occurred.
        // If yes, we return and print an error message
        // to the commandline.
        err.PrintErrorMsg();
        return (int)err.GetCode();
    }

    // The PDF was created sucessfully.
    std::cout << std::endl
        << "Created a PDF file containing the line \"Hello World!\": " << argv[1] << std::endl << std::endl;

    return 0;
}

// Base14 + other non-Base14 fonts for comparison

static const char* s_base14fonts[] =
{
    "Times-Roman",
    "Times-Italic",
    "Times-Bold",
    "Times-BoldItalic",
    "Helvetica",
    "Helvetica-Oblique",
    "Helvetica-Bold",
    "Helvetica-BoldOblique",
    "Courier",
    "Courier-Oblique",
    "Courier-Bold",
    "Courier-BoldOblique",
    "Symbol",
    "ZapfDingbats",
    "Arial",
    "Verdana"
};

const char* GetBase14FontName(unsigned i)
{
    if (i >= std::size(s_base14fonts))
        return nullptr;

    return s_base14fonts[i];
}

void DrawRedFrame(PdfPainter& painter, double x, double y, double width, double height)
{
    // draw red box
    painter.GetGraphicsState().SetFillColor(PdfColor(1.0f, 0.0f, 0.0f));
    painter.GetGraphicsState().SetStrokeColor(PdfColor(1.0f, 0.0f, 0.0f));
    painter.DrawLine(x, y, x + width, y);
    if (height > 0.0f)
    {
        painter.DrawLine(x, y, x, y + height);
        painter.DrawLine(x + width, y, x + width, y + height);
        painter.DrawLine(x, y + height, x + width, y + height);
    }
    // restore to black
    painter.GetGraphicsState().SetFillColor(PdfColor(0.0f, 0.0f, 0.0f));
    painter.GetGraphicsState().SetStrokeColor(PdfColor(0.0f, 0.0f, 0.0f));
}

void DemoBase14Fonts(PdfPainter& painter, PdfPage& page, PdfDocument& document, const PdfFontSearchParams& params)
{
    double x = 56, y = page.GetRect().GetHeight() - 56.69;
    string_view demo_text = "abcdefgABCDEFG12345!#$%&+-@?        ";
    double height = 0.0f, width = 0.0f;

    // draw sample of all types
    for (unsigned i = 0; i < std::size(s_base14fonts); i++)
    {
        x = 56; y = y - 25;
        string text = (string)demo_text;
        text.append(GetBase14FontName(i));

        PdfFont* font = document.GetFonts().SearchFont(GetBase14FontName(i), params);
        if (font == nullptr)
            throw runtime_error("Font not found");

        painter.GetTextState().SetFont(*font, 12.0);

        width = font->GetStringLength(text, painter.GetTextState());
        height = font->GetMetrics().GetLineSpacing();

        std::cout << GetBase14FontName(i) << " Width = " << width << " Height = " << height << std::endl;

        // draw red box
        DrawRedFrame(painter, x, y, width, height);

        // draw text
        painter.DrawText(text, x, y);
    }

    // draw some individual characters:
    string_view demo_text2 = " @_1jiPlg .;";

    auto font2 = document.GetFonts().SearchFont("Arial", params);
    auto& metrics = font2->GetMetrics();
    cout << "Non base 14 font characteristics" << endl;
    cout << "Font name: " << metrics.GetFontName() << endl;
    cout << "Family font name: " << metrics.GetFontFamilyName() << endl;
    cout << "Font file path: " << metrics.GetFilePath() << endl;
    cout << "Font face index: " << metrics.GetFaceIndex() << endl;

    // draw  individuals
    for (unsigned i = 0; i < demo_text2.length(); i++)
    {
        x = 56; y = y - 25;
        string text;
        if (i == 0)
            text = "Helvetica / Arial Comparison:";
        else
            text = (string)demo_text2.substr(i, 1);

        PdfFont* font = document.GetFonts().SearchFont("Helvetica", params);
        painter.GetTextState().SetFont(*font, 12);
        height = font->GetMetrics().GetLineSpacing();
        width = font->GetStringLength(text, painter.GetTextState());

        // draw red box
        DrawRedFrame(painter, x, y, width, height);

        // draw text
        painter.DrawText(text, x, y);

        if (i > 0)
        {
            // draw again, with non-Base14 font
            painter.GetTextState().SetFont(*font2, 12);
            height = font2->GetMetrics().GetLineSpacing();
            width = font2->GetStringLength((string_view)text, painter.GetTextState());

            // draw red box
            DrawRedFrame(painter, x + 100, y, width, height);

            // draw text
            painter.DrawText(text, x + 100, y);
        }
    }
}
