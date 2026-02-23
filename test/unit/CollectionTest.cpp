/**
 * Copyright (C) 2025 by David Lilly <david.lilly@ticketmaster.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("CreateCollection")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    REQUIRE_FALSE(doc.IsPortfolio());

    doc.GetOrCreateCollection();
    REQUIRE(doc.IsPortfolio());

    // Verify collection exists in catalog
    auto catalogCollection = doc.GetCatalog().GetCollectionObject();
    REQUIRE(catalogCollection != nullptr);

    doc.Save(TestUtils::GetTestOutputFilePath("CreateCollection.pdf"));
}

TEST_CASE("CollectionSchema")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    auto& collection = doc.GetOrCreateCollection();
    auto& schema = collection.GetOrCreateSchema();

    // Add fields to schema
    schema.AddField("Title", PdfCollectionFieldType::String, PdfString("Document Title"), static_cast<int64_t>(0));
    schema.AddField("Author", PdfCollectionFieldType::String, PdfString("Author Name"), static_cast<int64_t>(1));
    schema.AddField("Size", PdfCollectionFieldType::Number, PdfString("File Size"), static_cast<int64_t>(2));

    REQUIRE(schema.HasField("Title"));
    REQUIRE(schema.HasField("Author"));
    REQUIRE(schema.HasField("Size"));
    REQUIRE_FALSE(schema.HasField("NonExistent"));

    auto fieldNames = schema.GetFieldNames();
    REQUIRE(fieldNames.size() == 3);

    // Verify field types
    auto titleType = schema.GetFieldType("Title");
    REQUIRE(titleType != nullptr);
    REQUIRE(*titleType == PdfCollectionFieldType::String);

    auto sizeType = schema.GetFieldType("Size");
    REQUIRE(sizeType != nullptr);
    REQUIRE(*sizeType == PdfCollectionFieldType::Number);

    doc.Save(TestUtils::GetTestOutputFilePath("CollectionSchema.pdf"));
}

TEST_CASE("CollectionItem")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    auto& collection = doc.GetOrCreateCollection();
    auto& schema = collection.GetOrCreateSchema();
    schema.AddField("Title", PdfCollectionFieldType::String);
    schema.AddField("Count", PdfCollectionFieldType::Number);
    schema.AddField("Date", PdfCollectionFieldType::Date);

    // Create a file spec with collection item
    shared_ptr<PdfFileSpec> fs = doc.CreateFileSpec();
    fs->SetFilename(PdfString("test.txt"));
    fs->SetEmbeddedData(charbuff(string("Test content")));

    auto& item = fs->GetOrCreateCollectionItem();
    item.SetFieldValue("Title", PdfString("Test Document"));
    item.SetFieldValue("Count", 42.0);
    item.SetFieldValue("Date", PdfDate::LocalNow());

    // Verify values
    auto titleObj = item.GetFieldValue("Title");
    REQUIRE(titleObj != nullptr);
    REQUIRE(titleObj->GetString() == "Test Document");

    auto countObj = item.GetFieldValue("Count");
    REQUIRE(countObj != nullptr);
    REQUIRE(countObj->GetReal() == 42.0);

    auto dateObj = item.GetFieldValue("Date");
    REQUIRE(dateObj != nullptr);

    doc.Save(TestUtils::GetTestOutputFilePath("CollectionItem.pdf"));
}

TEST_CASE("FileSpecCollectionItem")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    auto& collection = doc.GetOrCreateCollection();
    auto& schema = collection.GetOrCreateSchema();
    schema.AddField("Description", PdfCollectionFieldType::String);

    shared_ptr<PdfFileSpec> fs = doc.CreateFileSpec();
    fs->SetFilename(PdfString("document.pdf"));
    fs->SetEmbeddedData(charbuff(string("PDF content")));

    // Add collection item
    auto& item = fs->GetOrCreateCollectionItem();
    item.SetFieldValue("Description", PdfString("Important document"));

    // Verify /CI key exists in filespec
    auto ciObj = fs->GetObject().GetDictionary().FindKey("CI");
    REQUIRE(ciObj != nullptr);

    // Verify can retrieve collection item
    auto retrievedItem = fs->GetCollectionItem();
    REQUIRE(retrievedItem != nullptr);

    auto desc = retrievedItem->GetFieldValue("Description");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->GetString() == "Important document");

    doc.Save(TestUtils::GetTestOutputFilePath("FileSpecCollectionItem.pdf"));
}

TEST_CASE("CompletePortfolio")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    // Create collection with schema
    auto& collection = doc.GetOrCreateCollection();
    auto& schema = collection.GetOrCreateSchema();
    schema.AddField("Title", PdfCollectionFieldType::String, PdfString("Title"), static_cast<int64_t>(0));
    schema.AddField("Author", PdfCollectionFieldType::String, PdfString("Author"), static_cast<int64_t>(1));
    schema.AddField("Size", PdfCollectionFieldType::Number, PdfString("Size"), static_cast<int64_t>(2));

    // Set view mode
    collection.SetViewMode(PdfCollectionViewMode::Details);

    // Add embedded files with metadata
    auto& names = doc.GetOrCreateNames();
    auto& embeddedFiles = names.GetOrCreateTree<PdfEmbeddedFiles>();

    for (int i = 1; i <= 3; i++)
    {
        shared_ptr<PdfFileSpec> fs = doc.CreateFileSpec();
        string filename = "file" + to_string(i) + ".txt";
        fs->SetFilename(PdfString(filename));
        fs->SetEmbeddedData(charbuff(string("Content " + to_string(i))));

        auto& item = fs->GetOrCreateCollectionItem();
        item.SetFieldValue("Title", PdfString("Document " + to_string(i)));
        item.SetFieldValue("Author", PdfString("Author " + to_string(i)));
        item.SetFieldValue("Size", static_cast<double>(10 + i));

        embeddedFiles.AddValue(*fs->GetFilename(), fs);
    }

    doc.Save(TestUtils::GetTestOutputFilePath("CompletePortfolio.pdf"));
}

TEST_CASE("LoadPortfolio")
{
    // Create portfolio first
    {
        PdfMemDocument doc;
        doc.GetPages().CreatePage();

        auto& collection = doc.GetOrCreateCollection();
        auto& schema = collection.GetOrCreateSchema();
        schema.AddField("Name", PdfCollectionFieldType::String);

        shared_ptr<PdfFileSpec> fs = doc.CreateFileSpec();
        fs->SetFilename(PdfString("data.txt"));
        fs->SetEmbeddedData(charbuff(string("Test data")));

        auto& item = fs->GetOrCreateCollectionItem();
        item.SetFieldValue("Name", PdfString("Test File"));

        auto& names = doc.GetOrCreateNames();
        auto& embeddedFiles = names.GetOrCreateTree<PdfEmbeddedFiles>();
        embeddedFiles.AddValue(*fs->GetFilename(), fs);

        doc.Save(TestUtils::GetTestOutputFilePath("LoadPortfolio.pdf"));
    }

    // Load and verify
    PdfMemDocument loadedDoc;
    loadedDoc.Load(TestUtils::GetTestOutputFilePath("LoadPortfolio.pdf"));

    REQUIRE(loadedDoc.IsPortfolio());

    auto collection = loadedDoc.GetCollection();
    REQUIRE(collection != nullptr);

    auto schema = collection->GetSchema();
    REQUIRE(schema != nullptr);
    REQUIRE(schema->HasField("Name"));

    // Verify embedded files
    auto names = loadedDoc.GetNames();
    REQUIRE(names != nullptr);
}

TEST_CASE("ViewModes")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    auto& collection = doc.GetOrCreateCollection();

    // Test Details view
    collection.SetViewMode(PdfCollectionViewMode::Details);
    REQUIRE(collection.GetViewMode() == PdfCollectionViewMode::Details);

    // Test Tile view
    collection.SetViewMode(PdfCollectionViewMode::Tile);
    REQUIRE(collection.GetViewMode() == PdfCollectionViewMode::Tile);

    // Test Hidden view
    collection.SetViewMode(PdfCollectionViewMode::Hidden);
    REQUIRE(collection.GetViewMode() == PdfCollectionViewMode::Hidden);

    doc.Save(TestUtils::GetTestOutputFilePath("ViewModes.pdf"));
}

TEST_CASE("SortConfiguration")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    auto& collection = doc.GetOrCreateCollection();
    auto& schema = collection.GetOrCreateSchema();
    schema.AddField("Title", PdfCollectionFieldType::String);
    schema.AddField("Date", PdfCollectionFieldType::Date);

    REQUIRE_FALSE(collection.HasSort());

    // Set sort by Title ascending
    collection.SetSort("Title", true);
    REQUIRE(collection.HasSort());

    // Change to sort by Date descending
    collection.SetSort("Date", false);
    REQUIRE(collection.HasSort());

    // Clear sorting
    collection.ClearSort();
    REQUIRE_FALSE(collection.HasSort());

    doc.Save(TestUtils::GetTestOutputFilePath("SortConfiguration.pdf"));
}

TEST_CASE("InitialDocument")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    auto& collection = doc.GetOrCreateCollection();

    auto initialDoc = collection.GetInitialDocument();
    REQUIRE(initialDoc == nullptr);

    collection.SetInitialDocument(PdfString("welcome.pdf"));
    initialDoc = collection.GetInitialDocument();
    REQUIRE(initialDoc != nullptr);
    REQUIRE(*initialDoc == "welcome.pdf");

    collection.SetInitialDocument(nullptr);
    initialDoc = collection.GetInitialDocument();
    REQUIRE(initialDoc == nullptr);

    doc.Save(TestUtils::GetTestOutputFilePath("InitialDocument.pdf"));
}

TEST_CASE("RemoveCollection")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    doc.GetOrCreateCollection();
    REQUIRE(doc.IsPortfolio());

    doc.RemoveCollection();
    REQUIRE_FALSE(doc.IsPortfolio());

    auto catalogCollection = doc.GetCatalog().GetCollectionObject();
    REQUIRE(catalogCollection == nullptr);

    doc.Save(TestUtils::GetTestOutputFilePath("RemoveCollection.pdf"));
}

TEST_CASE("AllFieldTypes")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    auto& collection = doc.GetOrCreateCollection();
    auto& schema = collection.GetOrCreateSchema();

    // Add all field types
    schema.AddField("String", PdfCollectionFieldType::String);
    schema.AddField("Date", PdfCollectionFieldType::Date);
    schema.AddField("Number", PdfCollectionFieldType::Number);
    schema.AddField("Filename", PdfCollectionFieldType::Filename);
    schema.AddField("Description", PdfCollectionFieldType::Description);
    schema.AddField("ModDate", PdfCollectionFieldType::ModDate);
    schema.AddField("CreationDate", PdfCollectionFieldType::CreationDate);
    schema.AddField("Size", PdfCollectionFieldType::Size);

    REQUIRE(schema.GetFieldNames().size() == 8);

    // Verify each type
    REQUIRE(*schema.GetFieldType("String") == PdfCollectionFieldType::String);
    REQUIRE(*schema.GetFieldType("Date") == PdfCollectionFieldType::Date);
    REQUIRE(*schema.GetFieldType("Number") == PdfCollectionFieldType::Number);
    REQUIRE(*schema.GetFieldType("Filename") == PdfCollectionFieldType::Filename);
    REQUIRE(*schema.GetFieldType("Description") == PdfCollectionFieldType::Description);
    REQUIRE(*schema.GetFieldType("ModDate") == PdfCollectionFieldType::ModDate);
    REQUIRE(*schema.GetFieldType("CreationDate") == PdfCollectionFieldType::CreationDate);
    REQUIRE(*schema.GetFieldType("Size") == PdfCollectionFieldType::Size);

    doc.Save(TestUtils::GetTestOutputFilePath("AllFieldTypes.pdf"));
}

TEST_CASE("EmptySchema")
{
    PdfMemDocument doc;
    doc.GetPages().CreatePage();

    doc.GetOrCreateCollection();

    // Collection without schema should work - verify it exists
    REQUIRE(doc.IsPortfolio());

    shared_ptr<PdfFileSpec> fs = doc.CreateFileSpec();
    fs->SetFilename(PdfString("file.txt"));
    fs->SetEmbeddedData(charbuff(string("Content")));

    auto& names = doc.GetOrCreateNames();
    auto& embeddedFiles = names.GetOrCreateTree<PdfEmbeddedFiles>();
    embeddedFiles.AddValue(*fs->GetFilename(), fs);

    doc.Save(TestUtils::GetTestOutputFilePath("EmptySchema.pdf"));
}
