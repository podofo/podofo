/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

TEST_CASE("TestImage1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestImage1.pdf"));
    auto& page = doc.GetPages().GetPageAt(0);
    auto& resources = page.MustGetResources();
    auto imageObj = resources.GetResource("XObject", "XOb5");
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

    string expectedImage;
    TestUtils::ReadTestInputFileTo(expectedImage, "ReferenceImage.ppm");

    REQUIRE(ppmbuffer == expectedImage);
}

TEST_CASE("TestImage2")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Hierarchies1.pdf"));
    // Try to extract jpeg image
    auto imageObj = doc.GetObjects().GetObject(PdfReference(36, 0));
    charbuff buffer;

    // Unpacking directly the stream shall throw since it has jpeg content
    ASSERT_THROW_WITH_ERROR_CODE(imageObj->GetStream()->CopyTo(buffer), PdfErrorCode::UnsupportedFilter);

    // Unpacking using UnpackToSafe() should succeed
    imageObj->GetStream()->CopyToSafe(buffer);

    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

#ifdef PODOFO_PLAYGROUND
    // NOTE: The following check may file using different,
    // jpeg libraries such as libjpeg-turbo
    string expectedImage;
    TestUtils::ReadTestInputFileTo(expectedImage, "ReferenceImage.ppm");

    REQUIRE(ppmbuffer == expectedImage);
#endif // PODOFO_PLAYGROUND
}

static void testReferenceImage(const PdfDocument& doc)
{
    auto& page = doc.GetPages().GetPageAt(0);
    auto resources = page.MustGetResources().GetResourceIterator("XObject");
    for (auto& res : resources)
    {
        unique_ptr<const PdfImage> image;
        REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*res.second, image));

        charbuff buffer;
        image->DecodeTo(buffer, PdfPixelFormat::BGRA);
        charbuff ppmbuffer;
        TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
            PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

        string expectedImage;
        TestUtils::ReadTestInputFileTo(expectedImage, "ReferenceImage.ppm");

        REQUIRE(ppmbuffer == expectedImage);

        break;
    }
}

TEST_CASE("TestImage3")
{
    auto outputFile = TestUtils::GetTestOutputFilePath("TestImage3.pdf");
    {
        PdfMemDocument doc;
        PdfPainter painter;
        auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        painter.SetCanvas(page);
        auto img = doc.CreateImage();
        img->Load(TestUtils::GetTestInputFilePath("ReferenceImage.png"));
        painter.DrawImage(*img.get(), 50.0, 50.0);
        painter.FinishDrawing();
        doc.Save(outputFile);
    }

    {
        PdfMemDocument doc;
        doc.Load(outputFile);
        testReferenceImage(doc);
    }
}

TEST_CASE("TestImage4")
{
    auto outputFile = TestUtils::GetTestOutputFilePath("TestImage4.pdf");
    {
        PdfMemDocument doc;
        PdfPainter painter;
        auto& page = doc.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
        painter.SetCanvas(page);
        auto img = doc.CreateImage();
        img->Load(TestUtils::GetTestInputFilePath("ReferenceImage.jpg"));
        auto alpha = doc.CreateImage();
        FileStreamDevice alphaInput(TestUtils::GetTestInputFilePath("ReferenceImage.alpha"));
        PdfImageInfo info;
        info.Width = 128;
        info.Height = 128;
        info.ColorSpace = PdfColorSpace::DeviceGray;
        info.BitsPerComponent = 8;
        alpha->SetDataRaw(alphaInput, info);
        img->SetSoftMask(*alpha);
        painter.DrawImage(*img.get(), 50.0, 50.0);
        painter.FinishDrawing();
        doc.Save(outputFile);
    }

#ifdef PODOFO_PLAYGROUND
    // NOTE: The following check may file using different,
    // jpeg libraries such as libjpeg-turbo
    {
        PdfMemDocument doc;
        doc.Load(outputFile);
        testReferenceImage(doc);
    }
#endif // PODOFO_PLAYGROUND
}
