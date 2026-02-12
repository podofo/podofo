/**
 * Copyright (C) 2025 by David Lilly <david.lilly@ticketmaster.com>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// This example demonstrates how to read and extract information
// from a PDF Portfolio (Collection) using PoDoFo.

#include <iostream>
#include <iomanip>
#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

void PrintHelp()
{
    cout << "This example reads a PDF Portfolio and displays its contents." << endl
        << "Please see https://github.com/podofo/podofo for more information" << endl << endl;
    cout << "Usage:" << endl;
    cout << "  read_portfolio <inputfile.pdf>" << endl << endl;
}

string GetViewModeName(PdfCollectionViewMode mode)
{
    switch (mode)
    {
        case PdfCollectionViewMode::Details: return "Details";
        case PdfCollectionViewMode::Tile: return "Tile";
        case PdfCollectionViewMode::Hidden: return "Hidden";
        default: return "Unknown";
    }
}

string GetFieldTypeName(PdfCollectionFieldType type)
{
    switch (type)
    {
        case PdfCollectionFieldType::String: return "String";
        case PdfCollectionFieldType::Date: return "Date";
        case PdfCollectionFieldType::Number: return "Number";
        case PdfCollectionFieldType::Filename: return "Filename";
        case PdfCollectionFieldType::Description: return "Description";
        case PdfCollectionFieldType::ModDate: return "ModDate";
        case PdfCollectionFieldType::CreationDate: return "CreationDate";
        case PdfCollectionFieldType::Size: return "Size";
        default: return "Unknown";
    }
}

void ReadPortfolio(const string_view& filename)
{
    try
    {
        // Load the PDF document
        PdfMemDocument document;
        document.Load(filename);

        cout << "=== PDF Portfolio Reader ===" << endl << endl;

        // Check if the document is a portfolio
        if (!document.IsPortfolio())
        {
            cout << "This PDF is not a portfolio." << endl;
            return;
        }

        cout << "✓ This PDF is a portfolio" << endl << endl;

        // Get the collection
        auto collection = document.GetCollection();
        if (collection == nullptr)
        {
            cout << "Error: Could not retrieve collection." << endl;
            return;
        }

        // Display view mode
        auto viewMode = collection->GetViewMode();
        cout << "View Mode: " << GetViewModeName(viewMode) << endl;

        // Display initial document if set
        auto initialDoc = collection->GetInitialDocument();
        if (initialDoc != nullptr)
        {
            cout << "Initial Document: " << initialDoc->GetString() << endl;
        }

        // Check if sorting is configured
        if (collection->HasSort())
        {
            cout << "Sorting: Enabled" << endl;
        }

        cout << endl;

        // Get and display the schema
        auto schema = collection->GetSchema();
        if (schema != nullptr)
        {
            auto fieldNames = schema->GetFieldNames();
            cout << "Schema Fields (" << fieldNames.size() << "):" << endl;
            cout << string(50, '-') << endl;

            for (const auto& fieldName : fieldNames)
            {
                auto fieldType = schema->GetFieldType(fieldName);
                if (fieldType != nullptr)
                {
                    cout << "  " << left << setw(20) << fieldName
                         << " : " << GetFieldTypeName(*fieldType) << endl;
                }
            }
            cout << endl;
        }
        else
        {
            cout << "No schema defined." << endl << endl;
        }

        // Get the embedded files
        auto names = document.GetNames();
        if (names == nullptr)
        {
            cout << "No embedded files found." << endl;
            return;
        }

        try
        {
            auto embeddedFiles = names->GetTree<PdfEmbeddedFiles>();

            // Iterate through embedded files
            PdfNameTree<PdfFileSpec>::Map filesMap;
            embeddedFiles->ToDictionary(filesMap);

            cout << "Embedded Files (" << filesMap.size() << "):" << endl;
            cout << string(70, '=') << endl;

            int fileNumber = 1;
            for (const auto& [key, fileSpec] : filesMap)
            {
                cout << endl << "File " << fileNumber++ << ": " << key.GetString() << endl;
                cout << string(70, '-') << endl;

                // Get filename
                auto filenameObj = fileSpec->GetFilename();
                if (filenameObj != nullptr)
                {
                    cout << "  Filename: " << filenameObj->GetString() << endl;
                }

                // Get embedded data size
                auto embeddedData = fileSpec->GetEmbeddedData();
                if (embeddedData != nullptr)
                {
                    cout << "  Size: " << embeddedData->size() << " bytes" << endl;
                }

                // Get collection item (metadata)
                auto collectionItem = fileSpec->GetCollectionItem();
                if (collectionItem != nullptr)
                {
                    auto metadataFields = collectionItem->GetFieldNames();
                    if (!metadataFields.empty())
                    {
                        cout << "  Metadata:" << endl;
                        for (const auto& field : metadataFields)
                        {
                            auto value = collectionItem->GetFieldValue(field);
                            if (value != nullptr)
                            {
                                cout << "    " << left << setw(15) << field << ": ";

                                // Try to display the value based on its type
                                if (value->IsString())
                                {
                                    cout << value->GetString().GetString();
                                }
                                else if (value->IsNumber())
                                {
                                    cout << value->GetReal();
                                }
                                else if (value->IsReference())
                                {
                                    cout << "[Reference]";
                                }
                                else
                                {
                                    cout << "[Complex value]";
                                }
                                cout << endl;
                            }
                        }
                    }
                }
            }

            cout << endl << "=== End of Portfolio ===" << endl;
        }
        catch (...)
        {
            cout << "No embedded files tree found." << endl;
        }
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
    // Check for correct arguments
    if (argc != 2)
    {
        if (argc > 1 && (string(argv[1]) == "-h" || string(argv[1]) == "--help"))
        {
            PrintHelp();
            return 0;
        }

        cerr << "Error: Missing input file" << endl;
        PrintHelp();
        return 1;
    }

    try
    {
        ReadPortfolio(argv[1]);
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}
