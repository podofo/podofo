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

TEST_CASE("TextExtraction1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));
    auto& page = doc.GetPages().GetPageAt(0);
    vector<PdfTextEntry> entries;
    page.ExtractTextTo(entries);

    REQUIRE(entries[0].Text == "MATLAB (an abbreviation of \"matrix laboratory\") is a proprietary multi-paradigm programming");
    ASSERT_EQUAL(entries[0].X, 29.000000232);
    ASSERT_EQUAL(entries[0].Y, 694.943905559551);
    REQUIRE(entries[1].Text == "language and numerical computing environment developed by MathWorks. MATLAB allows matrix");
    ASSERT_EQUAL(entries[1].X, 29.000000232);
    ASSERT_EQUAL(entries[1].Y, 684.920205479362);
    REQUIRE(entries[2].Text == "manipulations, plotting of functions and data, implementation of algorithms, creation of user");
    ASSERT_EQUAL(entries[2].X, 28.977805831822455);
    ASSERT_EQUAL(entries[2].Y, 674.89580539916642);
    REQUIRE(entries[3].Text == "interfaces, and interfacing with programs written in other languages.");
    ASSERT_EQUAL(entries[3].X, 29.000000232);
    ASSERT_EQUAL(entries[3].Y, 664.872605318981);
}

TEST_CASE("TextExtraction2")
{
    // Extraction with inline fonts
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TextExtraction2.pdf"));
    auto& page = doc.GetPages().GetPageAt(0);
    vector<PdfTextEntry> entries;
    page.ExtractTextTo(entries);
    REQUIRE(entries[0].Text == "Test text");
    ASSERT_EQUAL(entries[0].X, 31.199999999999999);
    ASSERT_EQUAL(entries[0].Y, 801.60000000000002);
}

TEST_CASE("TextExtraction3")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TextExtractionPredefinedCmap.pdf"));
    auto& page = doc.GetPages().GetPageAt(0);
    vector<PdfTextEntry> entries;
    page.ExtractTextTo(entries);

    REQUIRE(entries[0].Text == "全省环岛天然气管网尚未成型，东部部分建设滞后，管网缺乏统一规划，管道管径、设计压力参差不齐，省内支干");
    ASSERT_EQUAL(entries[0].X, 44.59);
    ASSERT_EQUAL(entries[0].Y, 406.7);
    REQUIRE(entries[1].Text == "线及支线长度不足、密度过小，难以实现省内资源的调度配置。城市天然气管网密度太小，应急储备设施的储备能力");
    ASSERT_EQUAL(entries[1].X, 42.52);
    ASSERT_EQUAL(entries[1].Y, 394.2);
    REQUIRE(entries[2].Text == "不足，供气的安全可靠性较差。天然气管网公平接入机制尚未建立和用气序列不合理，使得天然气供应安全难以得到");
    ASSERT_EQUAL(entries[2].X, 42.52);
    ASSERT_EQUAL(entries[2].Y, 381.7);
    REQUIRE(entries[3].Text == "有效保障。");
    ASSERT_EQUAL(entries[3].X, 42.52);
    ASSERT_EQUAL(entries[3].Y, 369.2);
    REQUIRE(entries[4].Text == "3.重点耗能行业能耗占比较大，产值占比较低");
    ASSERT_EQUAL(entries[4].X, 44.59);
    ASSERT_EQUAL(entries[4].Y, 344.2);

    auto font = doc.GetFonts().GetCachedFont(PdfReference(7, 0));
    REQUIRE(font != nullptr);
    auto str = PdfString::FromHexData("00205168770173af5c9b592971366c147ba17f515c1a672a6210578bff0c4e1c90e890e852065efa8bbe6ede540eff0c7ba17f517f3a4e4f7edf4e0089c45212ff0c7ba190537ba15f8430018bbe8ba1538b529b53c25dee4e0d9f50ff0c77015185652f5e72");
    REQUIRE(font->GetEncoding().ConvertToUtf8(str) == " 全省环岛天然气管网尚未成型，东部部分建设滞后，管网缺乏统一规划，管道管径、设计压力参差不齐，省内支干");
}

TEST_CASE("TextExtraction4")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));

    auto& page = doc.GetPages().GetPageAt(0);
    vector<PdfTextEntry> entries;
    bool abort = false;
    PdfTextExtractParams params = {};
    params.AbortCheck = [&](const AbortCheckInfo& info) {
        abort = info.ReadCount > 2;
        return abort;
    };
    page.ExtractTextTo(entries, params);

    REQUIRE(abort);
}
