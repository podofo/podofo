/**
 * Copyright (C) 2025 by David Lilly <david.lilly@ticketmaster.com>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// This example demonstrates how to create a PDF Portfolio (Collection)
// with embedded files and metadata using PoDoFo.

#include <iostream>
#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

void PrintHelp()
{
    cout << "This example creates a PDF Portfolio with embedded files and metadata." << endl
        << "Please see https://github.com/podofo/podofo for more information" << endl << endl;
    cout << "Usage:" << endl;
    cout << "  create_portfolio [outputfile.pdf]" << endl << endl;
}

void CreatePortfolio(const string_view& filename)
{
    try
    {
        // Create a new PDF document
        PdfMemDocument document;

        // Create at least one page (required for valid PDF)
        document.GetPages().CreatePage(PdfPageSize::A4);

        // Create the collection (portfolio)
        auto& collection = document.GetOrCreateCollection();

        // Define the metadata schema for files in the portfolio
        auto& schema = collection.GetOrCreateSchema();
        schema.AddField("Title", PdfCollectionFieldType::String,
                       PdfString("Document Title"), static_cast<int64_t>(0));
        schema.AddField("Author", PdfCollectionFieldType::String,
                       PdfString("Author"), static_cast<int64_t>(1));
        schema.AddField("Size", PdfCollectionFieldType::Number,
                       PdfString("File Size (KB)"), static_cast<int64_t>(2));
        schema.AddField("Date", PdfCollectionFieldType::Date,
                       PdfString("Modified"), static_cast<int64_t>(3));

        // Set the portfolio view mode to Details (shows files in a table)
        collection.SetViewMode(PdfCollectionViewMode::Details);

        // Configure sorting by Title in ascending order
        collection.SetSort("Title", true);

        // Get the embedded files tree where we'll add the files
        auto& names = document.GetOrCreateNames();
        auto& embeddedFiles = names.GetOrCreateTree<PdfEmbeddedFiles>();

        // Add three example files to the portfolio
        for (int i = 1; i <= 3; i++)
        {
            // Create a file specification
            shared_ptr<PdfFileSpec> fs = document.CreateFileSpec();

            // Set the filename
            string fileName = "document" + to_string(i) + ".txt";
            fs->SetFilename(PdfString(fileName));

            // Create some sample content
            string content = "This is the content of document " + to_string(i) + ".\n";
            content += "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n";
            content += "This demonstrates PDF Portfolio functionality in PoDoFo.\n";

            // Embed the content
            fs->SetEmbeddedData(charbuff(content));

            // Create collection item (metadata) for this file
            auto& item = fs->GetOrCreateCollectionItem();
            item.SetFieldValue("Title", PdfString("Document " + to_string(i)));
            item.SetFieldValue("Author", PdfString("Author " + to_string(i)));
            item.SetFieldValue("Size", static_cast<double>(content.length() / 1024.0));
            item.SetFieldValue("Date", PdfDate::LocalNow());

            // Add the file to the embedded files tree
            embeddedFiles.AddValue(*fs->GetFilename(), fs);

            cout << "Added: " << fileName << " (" << content.length() << " bytes)" << endl;
        }

        // Save the portfolio
        document.Save(filename);

        cout << endl << "Portfolio created successfully: " << filename << endl;
        cout << "Open this file in Adobe Acrobat to view the portfolio." << endl;
    }
    catch (PdfError& err)
    {
        cerr << "PoDoFo Error (code " << static_cast<int>(err.GetCode()) << ")" << endl;
        throw;
    }
    catch (exception& ex)
    {
        cerr << "Error: " << ex.what() << endl;
        throw;
    }
}

int main(int argc, char* argv[])
{
    // Set a default output filename if none provided
    string filename = "portfolio.pdf";

    // Check for help flag
    if (argc == 2)
    {
        string arg = argv[1];
        if (arg == "-h" || arg == "--help")
        {
            PrintHelp();
            return 0;
        }
        filename = arg;
    }
    else if (argc > 2)
    {
        cerr << "Error: Too many arguments" << endl;
        PrintHelp();
        return 1;
    }

    try
    {
        CreatePortfolio(filename);
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}
