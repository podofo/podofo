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
    auto& resources = page.GetResources();
    auto imageObj = resources.GetResource(PdfResourceType::XObject, "XOb5");
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

    string expectedImage;
    TestUtils::ReadTestInputFile("ReferenceImage.ppm", expectedImage);

    REQUIRE(ppmbuffer == expectedImage);
}

TEST_CASE("TestImage2")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("Hierarchies1.pdf"));
    // Try to extract jpeg image
    auto& imageObj = doc.GetObjects().MustGetObject(PdfReference(156, 0));
    charbuff buffer;

    // Unpacking directly the stream shall throw since it has jpeg content
    ASSERT_THROW_WITH_ERROR_CODE(imageObj.MustGetStream().CopyTo(buffer), PdfErrorCode::UnsupportedFilter);

    // Unpacking using UnpackToSafe() should succeed
    imageObj.MustGetStream().CopyToSafe(buffer);

    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(imageObj, image));

    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

#ifdef PODOFO_PLAYGROUND
    // NOTE: The following check may file using different,
    // jpeg libraries such as libjpeg-turbo
    string expectedImage;
    TestUtils::ReadTestInputFile("ReferenceImage.ppm", expectedImage);

    REQUIRE(ppmbuffer == expectedImage);
#endif // PODOFO_PLAYGROUND
}

static void testReferenceImage(const PdfDocument& doc)
{
    auto& page = doc.GetPages().GetPageAt(0);
    auto resources = page.GetResources().GetResourceIterator(PdfResourceType::XObject);
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
        TestUtils::ReadTestInputFile("ReferenceImage.ppm", expectedImage);

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
        auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
        painter.SetCanvas(page);
        auto img = doc.CreateImage();
        img->Load(TestUtils::GetTestInputFilePath("ReferenceImage.png"));
        painter.DrawImage(*img, 50.0, 50.0);
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
        auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
        painter.SetCanvas(page);
        auto img = doc.CreateImage();
        img->Load(TestUtils::GetTestInputFilePath("ReferenceImage.jpg"));
        auto alpha = doc.CreateImage();
        FileStreamDevice alphaInput(TestUtils::GetTestInputFilePath("ReferenceImage.alpha"));
        PdfImageInfo info;
        info.Width = 128;
        info.Height = 128;
        info.ColorSpace = PdfColorSpaceType::DeviceGray;
        info.BitsPerComponent = 8;
        alpha->SetDataRaw(alphaInput, info);
        img->SetSoftMask(*alpha);
        painter.DrawImage(*img.get(), 50.0, 50.0);
        painter.FinishDrawing();
        doc.Save(outputFile);
    }

#ifdef PODOFO_PLAYGROUND
    // NOTE: The following check may fail using different,
    // jpeg libraries such as libjpeg-turbo
    {
        PdfMemDocument doc;
        doc.Load(outputFile);
        testReferenceImage(doc);
    }
#endif // PODOFO_PLAYGROUND
}

// TODO: Hash test
TEST_CASE("TestImage5")
{
    {
        // Image found at:
        // https://github.com/tyranron/mozjpeg-sys-issue-23-example/blob/master/ignucius.jpg
        PdfMemDocument doc;
        doc.Load(TestUtils::GetTestInputFilePath("YCbCr-jpeg.pdf"));
        auto imageObj = doc.GetObjects().GetObject(PdfReference(11, 0));
        unique_ptr<PdfImage> image;
        REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

        charbuff buffer;
        image->DecodeTo(buffer, PdfPixelFormat::BGRA);
        charbuff ppmbuffer;
        TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
            PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

        TestUtils::WriteTestOutputFile(TestUtils::GetTestOutputFilePath("YCbCr-jpeg.ppm"), ppmbuffer);
    }

    {
        // Image found at:
        // https://bugzilla.redhat.com/show_bug.cgi?id=166460
        PdfMemDocument doc;
        doc.Load(TestUtils::GetTestInputFilePath("YCCK-jpeg.pdf"));
        auto imageObj = doc.GetObjects().GetObject(PdfReference(11, 0));
        unique_ptr<PdfImage> image;
        REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

        charbuff buffer;
        image->DecodeTo(buffer, PdfPixelFormat::BGRA);
        charbuff ppmbuffer;
        TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
            PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

        TestUtils::WriteTestOutputFile(TestUtils::GetTestOutputFilePath("YCCK-jpeg.ppm"), ppmbuffer);
    }
}

TEST_CASE("TestImage6")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestImage2.pdf"));
    auto& page = doc.GetPages().GetPageAt(0);
    auto& resources = page.GetResources();
    auto imageObj = resources.GetResource(PdfResourceType::XObject, "X0");
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

    TestUtils::WriteTestOutputFile(TestUtils::GetTestOutputFilePath("TestImage2.ppm"), ppmbuffer);
}

TEST_CASE("TestImage7")
{
    auto outputFile = TestUtils::GetTestOutputFilePath("TestImage7.pdf");
    PdfMemDocument doc;
    PdfPainter painter;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    painter.SetCanvas(page);

    auto img1 = doc.CreateImage();
    img1->Load(TestUtils::GetTestInputFilePath("MultipleFormats.tif"));
    painter.DrawImage(*img1, 50, 700, 0.5, 0.5);

    auto img2 = doc.CreateImage();
    img2->Load(TestUtils::GetTestInputFilePath("MultipleFormats.tif"), { 8 });
    painter.DrawImage(*img2, 50, 600, 0.5, 0.5);

    painter.FinishDrawing();
    doc.Save(outputFile);
}

TEST_CASE("TestImage8")
{
    auto outputFile = TestUtils::GetTestOutputFilePath("TestImage8.pdf");
    PdfMemDocument doc;
    PdfPainter painter;
    auto& page = doc.GetPages().CreatePage(PdfPageSize::A4);
    painter.SetCanvas(page);

    auto img = doc.CreateImage();
    auto metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 0 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::TopLeft);
    painter.DrawImage(*img, 50, 650, 0.05, 0.05);

    img = doc.CreateImage();
    metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 1 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::TopRight);
    painter.DrawImage(*img, 200, 650, 0.05, 0.05);

    img = doc.CreateImage();
    metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 2 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::BottomRight);
    painter.DrawImage(*img, 350, 650, 0.05, 0.05);

    img = doc.CreateImage();
    metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 3 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::BottomLeft);
    painter.DrawImage(*img, 50, 450, 0.05, 0.05);

    img = doc.CreateImage();
    metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 4 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::LeftTop);
    painter.DrawImage(*img, 200, 450, 0.05, 0.05);

    img = doc.CreateImage();
    metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 5 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::RightTop);
    painter.DrawImage(*img, 400, 450, 0.05, 0.05);

    img = doc.CreateImage();
    metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 6 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::RightBottom);
    painter.DrawImage(*img, 50, 250, 0.05, 0.05);

    img = doc.CreateImage();
    metadata = img->Load(TestUtils::GetTestInputFilePath("TestRotations.tif"), { 7 });
    REQUIRE(metadata.Orientation == PdfImageOrientation::LeftBottom);
    painter.DrawImage(*img, 250, 250, 0.05, 0.05);

    painter.FinishDrawing();
    doc.Save(outputFile);
}
