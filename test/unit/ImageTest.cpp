// SPDX-FileCopyrightText: 2008 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: MIT-0

#include <PdfTest.h>

using namespace std;
using namespace PoDoFo;

#include <podofo/private/OpenSSLInternal.h>

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
    TestUtils::ReadTestInputFileTo(expectedImage, "ReferenceImage.ppm");

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
    TestUtils::ReadTestInputFileTo(expectedImage, "ReferenceImage.ppm");

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
        TestUtils::ReadTestInputFileTo(expectedImage, "ReferenceImage.ppm");

        REQUIRE(ppmbuffer == expectedImage);
        return;
    }

    FAIL("Reference image not found");
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

TEST_CASE("TestImageInvalidLength")
{
    auto inputFile = TestUtils::GetTestInputFilePath("TestImageInvalidLength.pdf");
    PdfMemDocument doc;
    doc.Load(inputFile);
    testReferenceImage(doc);
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

        TestUtils::WriteTestOutputFileTo(TestUtils::GetTestOutputFilePath("YCbCr-jpeg.ppm"), ppmbuffer);
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

        TestUtils::WriteTestOutputFileTo(TestUtils::GetTestOutputFilePath("YCCK-jpeg.ppm"), ppmbuffer);
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

    TestUtils::WriteTestOutputFileTo(TestUtils::GetTestOutputFilePath("TestImage2.ppm"), ppmbuffer);
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

TEST_CASE("TestImage9")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestIndexedRGB.pdf"));

    auto& obj = doc.GetObjects().MustGetObject(PdfReference(27, 0));
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(obj, image));

    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

    REQUIRE(ssl::ComputeMD5Str(buffer) == "87A5BA219A8429733A1704044617F331");
}

TEST_CASE("TestImage10")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestIndexedRGB_OutOfPalette.pdf"));

    auto& obj = doc.GetObjects().MustGetObject(PdfReference(20, 0));
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(obj, image));

    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

    REQUIRE(ssl::ComputeMD5Str(buffer) == "2548A8F48BC5FC9521D689239259AC48");
}

TEST_CASE("TestImage11")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestImageSeparation.pdf"));
    auto& page = doc.GetPages().GetPageAt(0);
    auto& resources = page.GetResources();
    auto imageObj = resources.GetResource(PdfResourceType::XObject, "R17");
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

    auto outputFile = TestUtils::GetTestOutputFilePath("TestImage11.ppm");
    utls::WriteTo(outputFile, ppmbuffer);

    REQUIRE(image->GetWidth() == 367);
    REQUIRE(image->GetHeight() == 367);
    REQUIRE(ssl::ComputeMD5Str(buffer) == "5A8439C551C2772A70722843F0DCEF31");
}

TEST_CASE("TestBitsPerComponent1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TestBitsPerComponent1.pdf"));

    auto& obj = doc.GetObjects().MustGetObject(PdfReference(27, 0));
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(obj, image));

    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::BGRA);
    charbuff ppmbuffer;
    TestUtils::SaveFramePPM(ppmbuffer, buffer.data(),
        PdfPixelFormat::BGRA, image->GetWidth(), image->GetHeight());

    REQUIRE(ssl::ComputeMD5Str(buffer) == "C7C13471D8E5CF9E48637513D73FDCB5");
}

TEST_CASE("TestImageJpegDimensionMismatch")
{
    // The image declares /Width 2 /Height 2 but embeds a 16x16 JPEG. The
    // scan line is sized from the declared width, so decoding the larger
    // JPEG must be rejected instead of writing past it
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("JpegWidthMismatch.pdf"));
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    auto imageObj = resources.GetResource(PdfResourceType::XObject, "Im0");
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    charbuff buffer;
    ASSERT_THROW_WITH_ERROR_CODE(image->DecodeTo(buffer, PdfPixelFormat::BGRA),
        PdfErrorCode::UnsupportedImageFormat);
}

TEST_CASE("TestImageCcittWidthMismatch")
{
    // The image declares /Width 4000 but /DecodeParms /Columns 8, so the
    // decoder scan line is far smaller than the width read per row. Decoding
    // must be rejected instead of reading past the scan line
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("CcittWidthMismatch.pdf"));
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    auto imageObj = resources.GetResource(PdfResourceType::XObject, "Im0");
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    charbuff buffer;
    ASSERT_THROW_WITH_ERROR_CODE(image->DecodeTo(buffer, PdfPixelFormat::Grayscale),
        PdfErrorCode::UnsupportedImageFormat);
}

TEST_CASE("TestImageCcittTruncatedData")
{
    // The CCITT stream is empty, so the decoder returns no scan line
    // for any row: the missing rows are decoded as the decoder white,
    // which is inverted with /BlackIs1
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Im0 3 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Type/XObject/Subtype/Image/Width 8/Height 2/ColorSpace/DeviceGray/BitsPerComponent 1/Filter/CCITTFaxDecode/DecodeParms<</K -1/Columns 8>>/Length 0>>stream

endstream
endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";
    string_view pdfBlackIs1 = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Im0 3 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Type/XObject/Subtype/Image/Width 8/Height 2/ColorSpace/DeviceGray/BitsPerComponent 1/Filter/CCITTFaxDecode/DecodeParms<</K -1/Columns 8/BlackIs1 true>>/Length 0>>stream

endstream
endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(doc.GetObjects().MustGetObject(PdfReference(3, 0)), image));
    charbuff buffer;
    image->DecodeTo(buffer, PdfPixelFormat::Grayscale);
    REQUIRE(buffer == charbuff(string(16, '\xFF')));

    doc.LoadFromBuffer(pdfBlackIs1);
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(doc.GetObjects().MustGetObject(PdfReference(3, 0)), image));
    image->DecodeTo(buffer, PdfPixelFormat::Grayscale);
    REQUIRE(buffer == charbuff(string(16, '\x00')));
}

TEST_CASE("TestImageCcittInvalidColumns")
{
    // The negative /Columns is refused by the CCITT decoder,
    // which is then not created: decoding must be rejected
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Im0 3 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Type/XObject/Subtype/Image/Width 8/Height 2/ColorSpace/DeviceGray/BitsPerComponent 1/Filter/CCITTFaxDecode/DecodeParms<</K -1/Columns -1>>/Length 0>>stream

endstream
endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(doc.GetObjects().MustGetObject(PdfReference(3, 0)), image));

    charbuff buffer;
    ASSERT_THROW_WITH_ERROR_CODE(image->DecodeTo(buffer, PdfPixelFormat::Grayscale),
        PdfErrorCode::UnsupportedImageFormat);
}

TEST_CASE("TestImageNoColorSpace")
{
    // The image has neither /ColorSpace nor /ImageMask: the color space must
    // default to the Unknown filter instead of staying null, and decoding fails
    string_view pdf = R"(%PDF-1.7
1 0 obj<</Type/Catalog/Pages 2 0 R/Im0 3 0 R>>endobj
2 0 obj<</Type/Pages/Kids[]/Count 0>>endobj
3 0 obj<</Type/XObject/Subtype/Image/Width 1/Height 1/BitsPerComponent 8/Length 1>>stream
x
endstream
endobj
trailer<</Root 1 0 R/Size 4>>
startxref
999999
%%EOF)";

    PdfMemDocument doc;
    doc.LoadFromBuffer(pdf);
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(doc.GetObjects().MustGetObject(PdfReference(3, 0)), image));
    REQUIRE(image->GetColorSpace().GetType() == PdfColorSpaceType::Unknown);

    charbuff buffer;
    ASSERT_THROW_WITH_ERROR_CODE(image->DecodeTo(buffer, PdfPixelFormat::BGRA), PdfErrorCode::NotImplemented);
}

TEST_CASE("TestImageUndersizedStream")
{
    // A 65535x21846 RGB image whose stream holds a few bytes: the source size
    // check must reject it, also where the required size exceeds 32 bits
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("ImageUndersizedStream.pdf"));
    auto& resources = doc.GetPages().GetPageAt(0).GetResources();
    auto imageObj = resources.GetResource(PdfResourceType::XObject, "Im0");
    unique_ptr<PdfImage> image;
    REQUIRE(PdfXObject::TryCreateFromObject<PdfImage>(*imageObj, image));

    charbuff buffer;
    ASSERT_THROW_WITH_ERROR_CODE(image->DecodeTo(buffer, PdfPixelFormat::RGB24),
        PdfErrorCode::UnsupportedImageFormat);
}

TEST_CASE("TestImageTiffTooWide")
{
    // A 70000x1 TIFF exceeds the 65535 limit applied to PDF image dictionaries:
    // the loader must reject it, as a huge width wraps the scan line sizes
    PdfMemDocument doc;
    auto img = doc.CreateImage();
    ASSERT_THROW_WITH_ERROR_CODE(img->Load(TestUtils::GetTestInputFilePath("TiffTooWide.tif")),
        PdfErrorCode::UnsupportedImageFormat);
}

TEST_CASE("TestImageSetDataTooWide")
{
    // Setting the data directly is subject to the same 65535 limit
    PdfMemDocument doc;
    auto img = doc.CreateImage();
    ASSERT_THROW_WITH_ERROR_CODE(img->SetData(bufferview(), 70000, 1, PdfPixelFormat::Grayscale),
        PdfErrorCode::UnsupportedImageFormat);

    PdfImageInfo info;
    info.Width = 70000;
    info.Height = 1;
    info.BitsPerComponent = 8;
    info.ColorSpace = PdfColorSpaceType::DeviceGray;
    ASSERT_THROW_WITH_ERROR_CODE(img->SetDataRaw(bufferview(), info), PdfErrorCode::UnsupportedImageFormat);
}

TEST_CASE("TestImageTiffPaletteMultiSample")
{
    // A palette TIFF with 2 samples per pixel: the colormap holds one entry
    // per sample value, so sizing it from bits * samples read past it. Such
    // an image must be rejected
    PdfMemDocument doc;
    auto img = doc.CreateImage();
    ASSERT_THROW_WITH_ERROR_CODE(img->Load(TestUtils::GetTestInputFilePath("TiffPaletteMultiSample.tif")),
        PdfErrorCode::UnsupportedImageFormat);
}

TEST_CASE("TestImagePng1BppPaletteTransparency")
{
    // A 1 bpp palette PNG with transparency: the soft mask loop indexed the
    // row as 4 pixels per byte instead of 8, reading past the row
    PdfMemDocument doc;
    auto img = doc.CreateImage();
    REQUIRE_NOTHROW(img->Load(TestUtils::GetTestInputFilePath("Png1BppPalette.png")));
    REQUIRE(img->GetWidth() == 800);

    // The pixels alternate the transparent palette index 0 and the opaque index 1
    auto smaskObj = img->GetDictionary().FindKey("SMask");
    REQUIRE(smaskObj != nullptr);
    charbuff smask;
    smaskObj->MustGetStream().CopyTo(smask);
    string expected;
    for (unsigned i = 0; i < 400; i++)
        expected.append("\x00\xFF", 2);
    REQUIRE(smask == expected);
}

TEST_CASE("TestImagePngTruncated")
{
    // libpng reports the truncated data by jumping to the frame that set the
    // jump buffer, which must still be alive: the error must become a PdfError
    PdfMemDocument doc;
    auto img = doc.CreateImage();
    ASSERT_THROW_WITH_ERROR_CODE(img->Load(TestUtils::GetTestInputFilePath("PngTruncated.png")),
        PdfErrorCode::InvalidHandle);
}

TEST_CASE("TestXObjectFromObjectConflictingType")
{
    PdfMemDocument doc;
    auto form = doc.CreateXObjectForm(Rect(0, 0, 100, 100));

    // Without an explicit template argument the deduced type is const PdfImage:
    // the type filter must still apply
    unique_ptr<const PdfImage> constImage;
    REQUIRE(!PdfXObject::TryCreateFromObject(form->GetObject(), constImage));
    REQUIRE(constImage == nullptr);

    unique_ptr<const PdfXObjectForm> constForm;
    REQUIRE(PdfXObject::TryCreateFromObject(form->GetObject(), constForm));
    REQUIRE(constForm->GetType() == PdfXObjectType::Form);
}
