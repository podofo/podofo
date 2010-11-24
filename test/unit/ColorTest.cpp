/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ColorTest.h"
#include <podofo.h>
#include "cppunitextensions.h"

#include <map>
#include <utility>
#include <string>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ColorTest );

//#define DEBUG_INFO

void ColorTest::setUp()
{
}

void ColorTest::tearDown()
{
}

void ColorTest::testDefaultConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testDefaultConstructor" << std::endl;
#endif

    PdfColor color;

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_Unknown);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetAlternateColorSpace(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGrayScale(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetRed(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGreen(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlue(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCyan(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetMagenta(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetYellow(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlack(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetName(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetDensity(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieL(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieA(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieB(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToGrayScale(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToRGB(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToCMYK(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ToArray(), 
        PdfError, 
        ePdfError_CannotConvertColor);
}

void ColorTest::testGreyConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testGreyConstructor" << std::endl;
#endif

    const double GREY_VALUE = 0.123;
    PdfColor color(GREY_VALUE);

    ASSERT_TRUE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceGray);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetAlternateColorSpace(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_EQ(color.GetGrayScale(), GREY_VALUE);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetRed(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGreen(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlue(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCyan(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetMagenta(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetYellow(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlack(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetName(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetDensity(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieL(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieA(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieB(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_TRUE(color == color.ConvertToGrayScale());

    ASSERT_TRUE(PdfColor(GREY_VALUE, GREY_VALUE, GREY_VALUE) == color.ConvertToRGB());

    ASSERT_TRUE(color.ConvertToRGB().ConvertToCMYK() == color.ConvertToCMYK());

    const PdfArray COLOR_ARRAY = color.ToArray();
    ASSERT_TRUE(1 == COLOR_ARRAY.GetSize());
    ASSERT_TRUE(PdfObject(GREY_VALUE) == COLOR_ARRAY[0]);

}

void ColorTest::testGreyConstructorInvalid() 
{
#ifdef DEBUG_INFO
    std::cout << "testGreyConstructorInvalid" << std::endl;
#endif

    {
        const double GREY_VALUE = 1.01;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(GREY_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
        const double GREY_VALUE = -0.01;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(GREY_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }
}

void ColorTest::testRGBConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testRGBConstructor" << std::endl;
#endif

    const double R_VALUE = 0.023;
    const double G_VALUE = 0.345;
    const double B_VALUE = 0.678;
    PdfColor color(R_VALUE, G_VALUE, B_VALUE);

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_TRUE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceRGB);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetAlternateColorSpace(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGrayScale(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_EQ(color.GetRed(), R_VALUE);
    ASSERT_EQ(color.GetGreen(), G_VALUE);
    ASSERT_EQ(color.GetBlue(), B_VALUE);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCyan(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetMagenta(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetYellow(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlack(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetName(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetDensity(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieL(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieA(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieB(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_TRUE(PdfColor(0.299*R_VALUE + 0.587*G_VALUE + 0.114*B_VALUE) == color.ConvertToGrayScale());

    ASSERT_TRUE(PdfColor(R_VALUE, G_VALUE, B_VALUE) == color.ConvertToRGB());

    {
        double dBlack   = PDF_MIN( 1.0-R_VALUE, PDF_MIN( 1.0-G_VALUE, 1.0-B_VALUE ));
        double dCyan    = (1.0-R_VALUE-dBlack)  /(1.0-dBlack);
        double dMagenta = (1.0-G_VALUE-dBlack)/(1.0-dBlack);
        double dYellow  = (1.0-B_VALUE-dBlack) /(1.0-dBlack);
        
        ASSERT_TRUE(PdfColor( dCyan, dMagenta, dYellow, dBlack ) == color.ConvertToCMYK());
    }

    const PdfArray COLOR_ARRAY = color.ToArray();
    ASSERT_TRUE(3 == COLOR_ARRAY.GetSize());
    ASSERT_TRUE(PdfObject(R_VALUE) == COLOR_ARRAY[0]);
    ASSERT_TRUE(PdfObject(G_VALUE) == COLOR_ARRAY[1]);
    ASSERT_TRUE(PdfObject(B_VALUE) == COLOR_ARRAY[2]);
}


void ColorTest::testRGBConstructorInvalid() 
{
#ifdef DEBUG_INFO
    std::cout << "testRGBConstructorInvalid" << std::endl;
#endif

    {
        const double R_VALUE = 1.023;
        const double G_VALUE = 0.345;
        const double B_VALUE = 0.678;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(R_VALUE, G_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
        const double R_VALUE = 0.023;
        const double G_VALUE = 1.345;
        const double B_VALUE = 0.678;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(R_VALUE, G_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
        const double R_VALUE = 0.023;
        const double G_VALUE = 0.345;
        const double B_VALUE = 2.678;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(R_VALUE, G_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
        const double R_VALUE = -0.023;
        const double G_VALUE = 0.345;
        const double B_VALUE = 0.678;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(R_VALUE, G_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
        const double R_VALUE = 0.023;
        const double G_VALUE = -0.345;
        const double B_VALUE = 0.678;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(R_VALUE, G_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
        const double R_VALUE = 0.023;
        const double G_VALUE = 0.345;
        const double B_VALUE = -0.678;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(R_VALUE, G_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

}

void ColorTest::testCMYKConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testCMYKConstructor" << std::endl;
#endif

    const double C_VALUE = 0.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = 0.4;
    PdfColor color(C_VALUE, M_VALUE, Y_VALUE, B_VALUE);

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_TRUE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceCMYK);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetAlternateColorSpace(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGrayScale(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetRed(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGreen(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlue(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_EQ(color.GetCyan(), C_VALUE);
    ASSERT_EQ(color.GetMagenta(), M_VALUE);
    ASSERT_EQ(color.GetYellow(), Y_VALUE);
    ASSERT_EQ(color.GetBlack(), B_VALUE);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetName(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetDensity(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieL(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieA(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieB(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_TRUE(color.ConvertToRGB().ConvertToGrayScale() == color.ConvertToGrayScale());

    {
        double dRed   = C_VALUE * (1.0 - B_VALUE) + B_VALUE;
        double dGreen = M_VALUE * (1.0 - B_VALUE) + B_VALUE;
        double dBlue  = Y_VALUE * (1.0 - B_VALUE) + B_VALUE;
        
        ASSERT_TRUE(PdfColor( 1.0 - dRed, 1.0 - dGreen, 1.0 - dBlue ) == color.ConvertToRGB());
    }

    ASSERT_TRUE(PdfColor(C_VALUE, M_VALUE, Y_VALUE, B_VALUE) == color.ConvertToCMYK());

    const PdfArray COLOR_ARRAY = color.ToArray();
    ASSERT_TRUE(4 == COLOR_ARRAY.GetSize());
    ASSERT_TRUE(PdfObject(C_VALUE) == COLOR_ARRAY[0]);
    ASSERT_TRUE(PdfObject(M_VALUE) == COLOR_ARRAY[1]);
    ASSERT_TRUE(PdfObject(Y_VALUE) == COLOR_ARRAY[2]);
    ASSERT_TRUE(PdfObject(B_VALUE) == COLOR_ARRAY[3]);
}

void ColorTest::testCMYKConstructorInvalid() 
{
#ifdef DEBUG_INFO
    std::cout << "testCMYKConstructorInvalid" << std::endl;
#endif

    {
    const double C_VALUE = 1.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = 0.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
    const double C_VALUE = 0.1;
    const double M_VALUE = 1.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = 0.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
    const double C_VALUE = 0.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = 1.3;
    const double B_VALUE = 0.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
    const double C_VALUE = 0.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = 1.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
    const double C_VALUE = -0.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = 0.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
    const double C_VALUE = 0.1;
    const double M_VALUE = -0.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = 0.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
    const double C_VALUE = 0.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = -0.3;
    const double B_VALUE = 0.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

    {
    const double C_VALUE = 0.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = -0.4;
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            const PdfColor TEST_COLOR(C_VALUE, M_VALUE, Y_VALUE, B_VALUE),
            PdfError, 
            ePdfError_ValueOutOfRange);
    }

}

void ColorTest::testCopyConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testCopyConstructor" << std::endl;
#endif

    {
        const double GREY_VALUE = 0.123;
        PdfColor initialColor(GREY_VALUE);
        PdfColor color(initialColor);

        ASSERT_TRUE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_FALSE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceGray);

        ASSERT_EQ(color.GetGrayScale(), GREY_VALUE);
    }

    {
        const double R_VALUE = 0.023;
        const double G_VALUE = 0.345;
        const double B_VALUE = 0.678;
        PdfColor initialColor(R_VALUE, G_VALUE, B_VALUE);
        PdfColor color(initialColor);

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_TRUE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_FALSE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceRGB);

        ASSERT_EQ(color.GetRed(), R_VALUE);
        ASSERT_EQ(color.GetGreen(), G_VALUE);
        ASSERT_EQ(color.GetBlue(), B_VALUE);
    }

    {
        const double C_VALUE = 0.1;
        const double M_VALUE = 0.2;
        const double Y_VALUE = 0.3;
        const double B_VALUE = 0.4;
        PdfColor initialColor(C_VALUE, M_VALUE, Y_VALUE, B_VALUE);
        PdfColor color(initialColor);

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_TRUE(color.IsCMYK());
        ASSERT_FALSE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceCMYK);

        ASSERT_EQ(color.GetCyan(), C_VALUE);
        ASSERT_EQ(color.GetMagenta(), M_VALUE);
        ASSERT_EQ(color.GetYellow(), Y_VALUE);
        ASSERT_EQ(color.GetBlack(), B_VALUE);
    }
}

void ColorTest::testAssignmentOperator() 
{
#ifdef DEBUG_INFO
    std::cout << "testAssignmentOperator" << std::endl;
#endif

    {
        const double GREY_VALUE = 0.123;
        PdfColor initialColor(GREY_VALUE);
        PdfColor color;
        color  = initialColor;

        ASSERT_TRUE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_FALSE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceGray);

        ASSERT_EQ(color.GetGrayScale(), GREY_VALUE);
    }

    {
        const double R_VALUE = 0.023;
        const double G_VALUE = 0.345;
        const double B_VALUE = 0.678;
        PdfColor initialColor(R_VALUE, G_VALUE, B_VALUE);
        PdfColor color;
        color  = initialColor;

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_TRUE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_FALSE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceRGB);

        ASSERT_EQ(color.GetRed(), R_VALUE);
        ASSERT_EQ(color.GetGreen(), G_VALUE);
        ASSERT_EQ(color.GetBlue(), B_VALUE);
    }

    {
        const double C_VALUE = 0.1;
        const double M_VALUE = 0.2;
        const double Y_VALUE = 0.3;
        const double B_VALUE = 0.4;
        PdfColor initialColor(C_VALUE, M_VALUE, Y_VALUE, B_VALUE);
        PdfColor color;
        color  = initialColor;

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_TRUE(color.IsCMYK());
        ASSERT_FALSE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceCMYK);

        ASSERT_EQ(color.GetCyan(), C_VALUE);
        ASSERT_EQ(color.GetMagenta(), M_VALUE);
        ASSERT_EQ(color.GetYellow(), Y_VALUE);
        ASSERT_EQ(color.GetBlack(), B_VALUE);
    }
}

void ColorTest::testEqualsOperator() 
{
#ifdef DEBUG_INFO
    std::cout << "testEqualsOperator" << std::endl;
#endif

    //Grey test
    { //Positive
        const double GREY_VALUE = 0.123;
        PdfColor lColor(GREY_VALUE);
        PdfColor rColor(GREY_VALUE);

        ASSERT_TRUE(lColor == rColor);
    }

    { //Negative
        const double L_GREY_VALUE = 0.123;
        PdfColor lColor(L_GREY_VALUE);
        const double R_GREY_VALUE = 0.124;
        PdfColor rColor(R_GREY_VALUE);

        ASSERT_FALSE(L_GREY_VALUE == R_GREY_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

    //RGB tests
    { //Positive
        const double L_R_VALUE = 0.023;
        const double L_G_VALUE = 0.345;
        const double L_B_VALUE = 0.678;
        PdfColor lColor(L_R_VALUE, L_G_VALUE, L_B_VALUE);
        const double R_R_VALUE = 0.023;
        const double R_G_VALUE = 0.345;
        const double R_B_VALUE = 0.678;
        PdfColor rColor(R_R_VALUE, R_G_VALUE, R_B_VALUE);

        ASSERT_TRUE(L_R_VALUE == R_R_VALUE);
        ASSERT_TRUE(L_G_VALUE == R_G_VALUE);
        ASSERT_TRUE(L_B_VALUE == R_B_VALUE);
        ASSERT_TRUE(lColor == rColor);
    }

    { //Negative
        const double L_R_VALUE = 0.023;
        const double L_G_VALUE = 0.345;
        const double L_B_VALUE = 0.678;
        PdfColor lColor(L_R_VALUE, L_G_VALUE, L_B_VALUE);
        const double R_R_VALUE = 0.100;
        const double R_G_VALUE = 0.345;
        const double R_B_VALUE = 0.678;
        PdfColor rColor(R_R_VALUE, R_G_VALUE, R_B_VALUE);

        ASSERT_FALSE(L_R_VALUE == R_R_VALUE);
        ASSERT_TRUE(L_G_VALUE == R_G_VALUE);
        ASSERT_TRUE(L_B_VALUE == R_B_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

    { //Negative
        const double L_R_VALUE = 0.023;
        const double L_G_VALUE = 0.345;
        const double L_B_VALUE = 0.678;
        PdfColor lColor(L_R_VALUE, L_G_VALUE, L_B_VALUE);
        const double R_R_VALUE = 0.023;
        const double R_G_VALUE = 0.340;
        const double R_B_VALUE = 0.678;
        PdfColor rColor(R_R_VALUE, R_G_VALUE, R_B_VALUE);

        ASSERT_TRUE(L_R_VALUE == R_R_VALUE);
        ASSERT_FALSE(L_G_VALUE == R_G_VALUE);
        ASSERT_TRUE(L_B_VALUE == R_B_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

    { //Negative
        const double L_R_VALUE = 0.023;
        const double L_G_VALUE = 0.345;
        const double L_B_VALUE = 0.678;
        PdfColor lColor(L_R_VALUE, L_G_VALUE, L_B_VALUE);
        const double R_R_VALUE = 0.023;
        const double R_G_VALUE = 0.345;
        const double R_B_VALUE = 0.677;
        PdfColor rColor(R_R_VALUE, R_G_VALUE, R_B_VALUE);

        ASSERT_TRUE(L_R_VALUE == R_R_VALUE);
        ASSERT_TRUE(L_G_VALUE == R_G_VALUE);
        ASSERT_FALSE(L_B_VALUE == R_B_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

    //CMYB tests
    { //Positive
        const double L_C_VALUE = 0.1;
        const double L_M_VALUE = 0.2;
        const double L_Y_VALUE = 0.3;
        const double L_B_VALUE = 0.4;
        PdfColor lColor(L_C_VALUE, L_M_VALUE, L_Y_VALUE, L_B_VALUE);
        const double R_C_VALUE = 0.1;
        const double R_M_VALUE = 0.2;
        const double R_Y_VALUE = 0.3;
        const double R_B_VALUE = 0.4;
        PdfColor rColor(R_C_VALUE, R_M_VALUE, R_Y_VALUE, R_B_VALUE);

        ASSERT_TRUE(L_C_VALUE == R_C_VALUE);
        ASSERT_TRUE(L_M_VALUE == R_M_VALUE);
        ASSERT_TRUE(L_Y_VALUE == R_Y_VALUE);
        ASSERT_TRUE(L_B_VALUE == R_B_VALUE);
        ASSERT_TRUE(lColor == rColor);
    }

    { //Negative
        const double L_C_VALUE = 0.1;
        const double L_M_VALUE = 0.2;
        const double L_Y_VALUE = 0.3;
        const double L_B_VALUE = 0.4;
        PdfColor lColor(L_C_VALUE, L_M_VALUE, L_Y_VALUE, L_B_VALUE);
        const double R_C_VALUE = 0.11;
        const double R_M_VALUE = 0.2;
        const double R_Y_VALUE = 0.3;
        const double R_B_VALUE = 0.4;
        PdfColor rColor(R_C_VALUE, R_M_VALUE, R_Y_VALUE, R_B_VALUE);

        ASSERT_FALSE(L_C_VALUE == R_C_VALUE);
        ASSERT_TRUE(L_M_VALUE == R_M_VALUE);
        ASSERT_TRUE(L_Y_VALUE == R_Y_VALUE);
        ASSERT_TRUE(L_B_VALUE == R_B_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

    { //Negative
        const double L_C_VALUE = 0.1;
        const double L_M_VALUE = 0.2;
        const double L_Y_VALUE = 0.3;
        const double L_B_VALUE = 0.4;
        PdfColor lColor(L_C_VALUE, L_M_VALUE, L_Y_VALUE, L_B_VALUE);
        const double R_C_VALUE = 0.1;
        const double R_M_VALUE = 0.21;
        const double R_Y_VALUE = 0.3;
        const double R_B_VALUE = 0.4;
        PdfColor rColor(R_C_VALUE, R_M_VALUE, R_Y_VALUE, R_B_VALUE);

        ASSERT_TRUE(L_C_VALUE == R_C_VALUE);
        ASSERT_FALSE(L_M_VALUE == R_M_VALUE);
        ASSERT_TRUE(L_Y_VALUE == R_Y_VALUE);
        ASSERT_TRUE(L_B_VALUE == R_B_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

    { //Negative
        const double L_C_VALUE = 0.1;
        const double L_M_VALUE = 0.2;
        const double L_Y_VALUE = 0.31;
        const double L_B_VALUE = 0.4;
        PdfColor lColor(L_C_VALUE, L_M_VALUE, L_Y_VALUE, L_B_VALUE);
        const double R_C_VALUE = 0.1;
        const double R_M_VALUE = 0.2;
        const double R_Y_VALUE = 0.3;
        const double R_B_VALUE = 0.4;
        PdfColor rColor(R_C_VALUE, R_M_VALUE, R_Y_VALUE, R_B_VALUE);

        ASSERT_TRUE(L_C_VALUE == R_C_VALUE);
        ASSERT_TRUE(L_M_VALUE == R_M_VALUE);
        ASSERT_FALSE(L_Y_VALUE == R_Y_VALUE);
        ASSERT_TRUE(L_B_VALUE == R_B_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

    { //Negative
        const double L_C_VALUE = 0.1;
        const double L_M_VALUE = 0.2;
        const double L_Y_VALUE = 0.3;
        const double L_B_VALUE = 0.4;
        PdfColor lColor(L_C_VALUE, L_M_VALUE, L_Y_VALUE, L_B_VALUE);
        const double R_C_VALUE = 0.1;
        const double R_M_VALUE = 0.2;
        const double R_Y_VALUE = 0.3;
        const double R_B_VALUE = 0.45;
        PdfColor rColor(R_C_VALUE, R_M_VALUE, R_Y_VALUE, R_B_VALUE);

        ASSERT_TRUE(L_C_VALUE == R_C_VALUE);
        ASSERT_TRUE(L_M_VALUE == R_M_VALUE);
        ASSERT_TRUE(L_Y_VALUE == R_Y_VALUE);
        ASSERT_FALSE(L_B_VALUE == R_B_VALUE);
        ASSERT_FALSE(lColor == rColor);
    }

}

void ColorTest::testHexNames() 
{
#ifdef DEBUG_INFO
    std::cout << "testHexNames" << std::endl;
#endif

    {
        PdfColor rgb = PdfColor::FromString( "#FF0AEF");
        ASSERT_TRUE(rgb.IsRGB());
        ASSERT_EQ(static_cast<int>(rgb.GetRed() * 255.0), 0xFF);
        ASSERT_EQ(static_cast<int>(rgb.GetGreen() * 255.0), 0x0A);
        ASSERT_EQ(static_cast<int>(rgb.GetBlue() * 255.0), 0xEF);
    }

    {
        PdfColor rgb = PdfColor::FromString( "#012345");
        ASSERT_TRUE(rgb.IsRGB());
        ASSERT_EQ(static_cast<int>(rgb.GetRed() * 255.0), 0x01);
        ASSERT_EQ(static_cast<int>(rgb.GetGreen() * 255.0), 0x23);
        ASSERT_EQ(static_cast<int>(rgb.GetBlue() * 255.0), 0x45);
    }

    {
        PdfColor rgb = PdfColor::FromString( "#ABCDEF");
        ASSERT_TRUE(rgb.IsRGB());
        ASSERT_EQ(static_cast<int>(rgb.GetRed() * 255.0), 0xAB);
        ASSERT_EQ(static_cast<int>(rgb.GetGreen() * 255.0), 0xCD);
        ASSERT_EQ(static_cast<int>(rgb.GetBlue() * 255.0), 0xEF);
    }

    {
        PdfColor rgb = PdfColor::FromString( "#abcdef");
        ASSERT_TRUE(rgb.IsRGB());
        ASSERT_EQ(static_cast<int>(rgb.GetRed() * 255.0), 0xAB);
        ASSERT_EQ(static_cast<int>(rgb.GetGreen() * 255.0), 0xCD);
        ASSERT_EQ(static_cast<int>(rgb.GetBlue() * 255.0), 0xEF);
    }

    {
        PdfColor invalidColour = PdfColor::FromString( "#01");
        ASSERT_TRUE(invalidColour == PdfColor());
    }

    {
        PdfColor invalidColour = PdfColor::FromString( "#123456789");
        ASSERT_TRUE(invalidColour == PdfColor());
    }

    {
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            PdfColor::FromString( "#12345g" ), 
            PdfError, 
            ePdfError_CannotConvertColor);
    }

    {
        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            PdfColor::FromString( "#1234g5" ), 
            PdfError, 
            ePdfError_CannotConvertColor);
    }

    PdfColor cmyk = PdfColor::FromString( "#ABCDEF01");
    ASSERT_TRUE(cmyk.IsCMYK());
    ASSERT_EQ(static_cast<int>(cmyk.GetCyan() * 255.0), 0xAB);
    ASSERT_EQ(static_cast<int>(cmyk.GetMagenta() * 255.0), 0xCD);
    ASSERT_EQ(static_cast<int>(cmyk.GetYellow() * 255.0), 0xEF);
    ASSERT_EQ(static_cast<int>(cmyk.GetBlack() * 255.0), 0x01);
}

void ColorTest::testNamesGeneral()
{
#ifdef DEBUG_INFO
    std::cout << "testNames" << std::endl;
#endif

    PdfColor aliceBlue = PdfColor::FromString( "aliceblue");
    ASSERT_TRUE(aliceBlue == PdfColor::FromString("#F0F8FF"));
    ASSERT_EQ(aliceBlue.GetRed(),   static_cast<double>(0xF0)/255.0);
    ASSERT_EQ(aliceBlue.GetGreen(), static_cast<double>(0xF8)/255.0);
    ASSERT_EQ(aliceBlue.GetBlue(),  static_cast<double>(0xFF)/255.0);

    PdfColor lime = PdfColor::FromString( "lime");
    ASSERT_TRUE(lime == PdfColor(0.000, 1.000, 0.000));

    PdfColor yellowGreen = PdfColor::FromString( "yellowgreen");
    ASSERT_TRUE(yellowGreen == PdfColor::FromString("#9ACD32"));

    {
        // Test a not existing color
        PdfColor notExist = PdfColor::FromString( "asfaf9q341");
        ASSERT_TRUE(notExist == PdfColor());
    }

    {
        // Test a not existing color
        PdfColor notExist = PdfColor::FromString( "A");
        ASSERT_TRUE(notExist == PdfColor());
    }

    {
        // Test a not existing color
        PdfColor notExist = PdfColor::FromString( "");
        ASSERT_TRUE(notExist == PdfColor());
    }

    {
        // Test a not existing color
        PdfColor notExist = PdfColor::FromString( "yellowgree");
        ASSERT_TRUE(notExist == PdfColor());
    }

    {
        // Test a not existing color
        PdfColor notExist = PdfColor::FromString( "yellowgreem");
        ASSERT_TRUE(notExist == PdfColor());
    }

    {
        // Test a not existing color
        PdfColor notExist = PdfColor::FromString( "yellowgreen ");
        ASSERT_TRUE(notExist == PdfColor());
    }
}

namespace
{
class TestColor
{
public:
    TestColor(int r, int g, int b, const char* colorName) :
        m_r(static_cast<double>(r)/255.0),
        m_g(static_cast<double>(g)/255.0),
        m_b(static_cast<double>(b)/255.0),
        m_colorName(colorName)
    {
        //do nothing
    }

    ~TestColor() {}

    double getR() const { return m_r; }
    double getG() const { return m_g; }
    double getB() const { return m_b; }
    const char* getColorName() const { return m_colorName; }

    TestColor(const TestColor& rhs) :
        m_r(rhs.m_r),
        m_g(rhs.m_g),
        m_b(rhs.m_b),
        m_colorName(rhs.m_colorName)
    {
        //do nothing
    }

private:
    TestColor();
    TestColor& operator=(const TestColor&);

    double m_r;
    double m_g;
    double m_b;
    const char* m_colorName;
};

}

void ColorTest::testNamesOneByOne()
{
#ifdef DEBUG_INFO
    std::cout << "testNames" << std::endl;
#endif


//Copied and adjusted from http://cvsweb.xfree86.org/cvsweb/xc/programs/rgb/rgb.txt?rev=1.2
    const TestColor TABLE_OF_TEST_COLORS[] = 
    {
        TestColor(255, 250, 250, "snow"),
        TestColor(248, 248, 255, "GhostWhite"),
        TestColor(245, 245, 245, "WhiteSmoke"),
        TestColor(220, 220, 220, "gainsboro"),
        TestColor(255, 250, 240, "FloralWhite"),
        TestColor(253, 245, 230, "OldLace"),
        TestColor(250, 240, 230, "linen"),
        TestColor(250, 235, 215, "AntiqueWhite"),
        TestColor(255, 239, 213, "PapayaWhip"),
        TestColor(255, 235, 205, "BlanchedAlmond"),
        TestColor(255, 228, 196, "bisque"),
        TestColor(255, 218, 185, "PeachPuff"),
        TestColor(255, 222, 173, "NavajoWhite"),
        TestColor(255, 228, 181, "moccasin"),
        TestColor(255, 248, 220, "cornsilk"),
        TestColor(255, 255, 240, "ivory"),
        TestColor(255, 250, 205, "LemonChiffon"),
        TestColor(255, 245, 238, "seashell"),
        TestColor(240, 255, 240, "honeydew"),
        TestColor(245, 255, 250, "MintCream"),
        TestColor(240, 255, 255, "azure"),
        TestColor(240, 248, 255, "AliceBlue"),
        TestColor(230, 230, 250, "lavender"),
        TestColor(255, 240, 245, "LavenderBlush"),
        TestColor(255, 228, 225, "MistyRose"),
        TestColor(255, 255, 255, "white"),
        TestColor(0, 0, 0, "black"),
        TestColor(47, 79, 79, "DarkSlateGray"),
        TestColor(47, 79, 79, "DarkSlateGrey"),
        TestColor(105, 105, 105, "DimGray"),
        TestColor(105, 105, 105, "DimGrey"),
        TestColor(112, 128, 144, "SlateGray"),
        TestColor(112, 128, 144, "SlateGrey"),
        TestColor(119, 136, 153, "LightSlateGray"),
        TestColor(119, 136, 153, "LightSlateGrey"),
        TestColor(190, 190, 190, "gray"),
        TestColor(190, 190, 190, "grey"),
        TestColor(211, 211, 211, "LightGrey"),
        TestColor(211, 211, 211, "LightGray"),
        TestColor(25, 25, 112, "MidnightBlue"),
        TestColor(0, 0, 128, "navy"),
        TestColor(0, 0, 128, "NavyBlue"),
        TestColor(100, 149, 237, "CornflowerBlue"),
        TestColor(72, 61, 139, "DarkSlateBlue"),
        TestColor(106, 90, 205, "SlateBlue"),
        TestColor(123, 104, 238, "MediumSlateBlue"),
        TestColor(132, 112, 255, "LightSlateBlue"),
        TestColor(0, 0, 205, "MediumBlue"),
        TestColor(65, 105, 225, "RoyalBlue"),
        TestColor(0, 0, 255, "blue"),
        TestColor(30, 144, 255, "DodgerBlue"),
        TestColor(0, 191, 255, "DeepSkyBlue"),
        TestColor(135, 206, 235, "SkyBlue"),
        TestColor(135, 206, 250, "LightSkyBlue"),
        TestColor(70, 130, 180, "SteelBlue"),
        TestColor(176, 196, 222, "LightSteelBlue"),
        TestColor(173, 216, 230, "LightBlue"),
        TestColor(176, 224, 230, "PowderBlue"),
        TestColor(175, 238, 238, "PaleTurquoise"),
        TestColor(0, 206, 209, "DarkTurquoise"),
        TestColor(72, 209, 204, "MediumTurquoise"),
        TestColor(64, 224, 208, "turquoise"),
        TestColor(0, 255, 255, "cyan"),
        TestColor(224, 255, 255, "LightCyan"),
        TestColor(95, 158, 160, "CadetBlue"),
        TestColor(102, 205, 170, "MediumAquamarine"),
        TestColor(127, 255, 212, "aquamarine"),
        TestColor(0, 100, 0, "DarkGreen"),
        TestColor(85, 107, 47, "DarkOliveGreen"),
        TestColor(143, 188, 143, "DarkSeaGreen"),
        TestColor(46, 139, 87, "SeaGreen"),
        TestColor(60, 179, 113, "MediumSeaGreen"),
        TestColor(32, 178, 170, "LightSeaGreen"),
        TestColor(152, 251, 152, "PaleGreen"),
        TestColor(0, 255, 127, "SpringGreen"),
        TestColor(124, 252, 0, "LawnGreen"),
        TestColor(0, 255, 0, "green"),
        TestColor(127, 255, 0, "chartreuse"),
        TestColor(0, 250, 154, "MediumSpringGreen"),
        TestColor(173, 255, 47, "GreenYellow"),
        TestColor(50, 205, 50, "LimeGreen"),
        TestColor(154, 205, 50, "YellowGreen"),
        TestColor(34, 139, 34, "ForestGreen"),
        TestColor(107, 142, 35, "OliveDrab"),
        TestColor(189, 183, 107, "DarkKhaki"),
        TestColor(240, 230, 140, "khaki"),
        TestColor(238, 232, 170, "PaleGoldenrod"),
        TestColor(250, 250, 210, "LightGoldenrodYellow"),
        TestColor(255, 255, 224, "LightYellow"),
        TestColor(255, 255, 0, "yellow"),
        TestColor(255, 215, 0, "gold"),
        TestColor(238, 221, 130, "LightGoldenrod"),
        TestColor(218, 165, 32, "goldenrod"),
        TestColor(184, 134, 11, "DarkGoldenrod"),
        TestColor(188, 143, 143, "RosyBrown"),
        TestColor(205, 92, 92, "IndianRed"),
        TestColor(139, 69, 19, "SaddleBrown"),
        TestColor(160, 82, 45, "sienna"),
        TestColor(205, 133, 63, "peru"),
        TestColor(222, 184, 135, "burlywood"),
        TestColor(245, 245, 220, "beige"),
        TestColor(245, 222, 179, "wheat"),
        TestColor(244, 164, 96, "SandyBrown"),
        TestColor(210, 180, 140, "tan"),
        TestColor(210, 105, 30, "chocolate"),
        TestColor(178, 34, 34, "firebrick"),
        TestColor(165, 42, 42, "brown"),
        TestColor(233, 150, 122, "DarkSalmon"),
        TestColor(250, 128, 114, "salmon"),
        TestColor(255, 160, 122, "LightSalmon"),
        TestColor(255, 165, 0, "orange"),
        TestColor(255, 140, 0, "DarkOrange"),
        TestColor(255, 127, 80, "coral"),
        TestColor(240, 128, 128, "LightCoral"),
        TestColor(255, 99, 71, "tomato"),
        TestColor(255, 69, 0, "OrangeRed"),
        TestColor(255, 0, 0, "red"),
        TestColor(255, 105, 180, "HotPink"),
        TestColor(255, 20, 147, "DeepPink"),
        TestColor(255, 192, 203, "pink"),
        TestColor(255, 182, 193, "LightPink"),
        TestColor(219, 112, 147, "PaleVioletRed"),
        TestColor(176, 48, 96, "maroon"),
        TestColor(199, 21, 133, "MediumVioletRed"),
        TestColor(208, 32, 144, "VioletRed"),
        TestColor(255, 0, 255, "magenta"),
        TestColor(238, 130, 238, "violet"),
        TestColor(221, 160, 221, "plum"),
        TestColor(218, 112, 214, "orchid"),
        TestColor(186, 85, 211, "MediumOrchid"),
        TestColor(153, 50, 204, "DarkOrchid"),
        TestColor(148, 0, 211, "DarkViolet"),
        TestColor(138, 43, 226, "BlueViolet"),
        TestColor(160, 32, 240, "purple"),
        TestColor(147, 112, 219, "MediumPurple"),
        TestColor(216, 191, 216, "thistle"),
        TestColor(255, 250, 250, "snow1"),
        TestColor(238, 233, 233, "snow2"),
        TestColor(205, 201, 201, "snow3"),
        TestColor(139, 137, 137, "snow4"),
        TestColor(255, 245, 238, "seashell1"),
        TestColor(238, 229, 222, "seashell2"),
        TestColor(205, 197, 191, "seashell3"),
        TestColor(139, 134, 130, "seashell4"),
        TestColor(255, 239, 219, "AntiqueWhite1"),
        TestColor(238, 223, 204, "AntiqueWhite2"),
        TestColor(205, 192, 176, "AntiqueWhite3"),
        TestColor(139, 131, 120, "AntiqueWhite4"),
        TestColor(255, 228, 196, "bisque1"),
        TestColor(238, 213, 183, "bisque2"),
        TestColor(205, 183, 158, "bisque3"),
        TestColor(139, 125, 107, "bisque4"),
        TestColor(255, 218, 185, "PeachPuff1"),
        TestColor(238, 203, 173, "PeachPuff2"),
        TestColor(205, 175, 149, "PeachPuff3"),
        TestColor(139, 119, 101, "PeachPuff4"),
        TestColor(255, 222, 173, "NavajoWhite1"),
        TestColor(238, 207, 161, "NavajoWhite2"),
        TestColor(205, 179, 139, "NavajoWhite3"),
        TestColor(139, 121, 94, "NavajoWhite4"),
        TestColor(255, 250, 205, "LemonChiffon1"),
        TestColor(238, 233, 191, "LemonChiffon2"),
        TestColor(205, 201, 165, "LemonChiffon3"),
        TestColor(139, 137, 112, "LemonChiffon4"),
        TestColor(255, 248, 220, "cornsilk1"),
        TestColor(238, 232, 205, "cornsilk2"),
        TestColor(205, 200, 177, "cornsilk3"),
        TestColor(139, 136, 120, "cornsilk4"),
        TestColor(255, 255, 240, "ivory1"),
        TestColor(238, 238, 224, "ivory2"),
        TestColor(205, 205, 193, "ivory3"),
        TestColor(139, 139, 131, "ivory4"),
        TestColor(240, 255, 240, "honeydew1"),
        TestColor(224, 238, 224, "honeydew2"),
        TestColor(193, 205, 193, "honeydew3"),
        TestColor(131, 139, 131, "honeydew4"),
        TestColor(255, 240, 245, "LavenderBlush1"),
        TestColor(238, 224, 229, "LavenderBlush2"),
        TestColor(205, 193, 197, "LavenderBlush3"),
        TestColor(139, 131, 134, "LavenderBlush4"),
        TestColor(255, 228, 225, "MistyRose1"),
        TestColor(238, 213, 210, "MistyRose2"),
        TestColor(205, 183, 181, "MistyRose3"),
        TestColor(139, 125, 123, "MistyRose4"),
        TestColor(240, 255, 255, "azure1"),
        TestColor(224, 238, 238, "azure2"),
        TestColor(193, 205, 205, "azure3"),
        TestColor(131, 139, 139, "azure4"),
        TestColor(131, 111, 255, "SlateBlue1"),
        TestColor(122, 103, 238, "SlateBlue2"),
        TestColor(105, 89, 205, "SlateBlue3"),
        TestColor(71, 60, 139, "SlateBlue4"),
        TestColor(72, 118, 255, "RoyalBlue1"),
        TestColor(67, 110, 238, "RoyalBlue2"),
        TestColor(58, 95, 205, "RoyalBlue3"),
        TestColor(39, 64, 139, "RoyalBlue4"),
        TestColor(0, 0, 255, "blue1"),
        TestColor(0, 0, 238, "blue2"),
        TestColor(0, 0, 205, "blue3"),
        TestColor(0, 0, 139, "blue4"),
        TestColor(30, 144, 255, "DodgerBlue1"),
        TestColor(28, 134, 238, "DodgerBlue2"),
        TestColor(24, 116, 205, "DodgerBlue3"),
        TestColor(16, 78, 139, "DodgerBlue4"),
        TestColor(99, 184, 255, "SteelBlue1"),
        TestColor(92, 172, 238, "SteelBlue2"),
        TestColor(79, 148, 205, "SteelBlue3"),
        TestColor(54, 100, 139, "SteelBlue4"),
        TestColor(0, 191, 255, "DeepSkyBlue1"),
        TestColor(0, 178, 238, "DeepSkyBlue2"),
        TestColor(0, 154, 205, "DeepSkyBlue3"),
        TestColor(0, 104, 139, "DeepSkyBlue4"),
        TestColor(135, 206, 255, "SkyBlue1"),
        TestColor(126, 192, 238, "SkyBlue2"),
        TestColor(108, 166, 205, "SkyBlue3"),
        TestColor(74, 112, 139, "SkyBlue4"),
        TestColor(176, 226, 255, "LightSkyBlue1"),
        TestColor(164, 211, 238, "LightSkyBlue2"),
        TestColor(141, 182, 205, "LightSkyBlue3"),
        TestColor(96, 123, 139, "LightSkyBlue4"),
        TestColor(198, 226, 255, "SlateGray1"),
        TestColor(185, 211, 238, "SlateGray2"),
        TestColor(159, 182, 205, "SlateGray3"),
        TestColor(108, 123, 139, "SlateGray4"),
        TestColor(202, 225, 255, "LightSteelBlue1"),
        TestColor(188, 210, 238, "LightSteelBlue2"),
        TestColor(162, 181, 205, "LightSteelBlue3"),
        TestColor(110, 123, 139, "LightSteelBlue4"),
        TestColor(191, 239, 255, "LightBlue1"),
        TestColor(178, 223, 238, "LightBlue2"),
        TestColor(154, 192, 205, "LightBlue3"),
        TestColor(104, 131, 139, "LightBlue4"),
        TestColor(224, 255, 255, "LightCyan1"),
        TestColor(209, 238, 238, "LightCyan2"),
        TestColor(180, 205, 205, "LightCyan3"),
        TestColor(122, 139, 139, "LightCyan4"),
        TestColor(187, 255, 255, "PaleTurquoise1"),
        TestColor(174, 238, 238, "PaleTurquoise2"),
        TestColor(150, 205, 205, "PaleTurquoise3"),
        TestColor(102, 139, 139, "PaleTurquoise4"),
        TestColor(152, 245, 255, "CadetBlue1"),
        TestColor(142, 229, 238, "CadetBlue2"),
        TestColor(122, 197, 205, "CadetBlue3"),
        TestColor(83, 134, 139, "CadetBlue4"),
        TestColor(0, 245, 255, "turquoise1"),
        TestColor(0, 229, 238, "turquoise2"),
        TestColor(0, 197, 205, "turquoise3"),
        TestColor(0, 134, 139, "turquoise4"),
        TestColor(0, 255, 255, "cyan1"),
        TestColor(0, 238, 238, "cyan2"),
        TestColor(0, 205, 205, "cyan3"),
        TestColor(0, 139, 139, "cyan4"),
        TestColor(151, 255, 255, "DarkSlateGray1"),
        TestColor(141, 238, 238, "DarkSlateGray2"),
        TestColor(121, 205, 205, "DarkSlateGray3"),
        TestColor(82, 139, 139, "DarkSlateGray4"),
        TestColor(127, 255, 212, "aquamarine1"),
        TestColor(118, 238, 198, "aquamarine2"),
        TestColor(102, 205, 170, "aquamarine3"),
        TestColor(69, 139, 116, "aquamarine4"),
        TestColor(193, 255, 193, "DarkSeaGreen1"),
        TestColor(180, 238, 180, "DarkSeaGreen2"),
        TestColor(155, 205, 155, "DarkSeaGreen3"),
        TestColor(105, 139, 105, "DarkSeaGreen4"),
        TestColor(84, 255, 159, "SeaGreen1"),
        TestColor(78, 238, 148, "SeaGreen2"),
        TestColor(67, 205, 128, "SeaGreen3"),
        TestColor(46, 139, 87, "SeaGreen4"),
        TestColor(154, 255, 154, "PaleGreen1"),
        TestColor(144, 238, 144, "PaleGreen2"),
        TestColor(124, 205, 124, "PaleGreen3"),
        TestColor(84, 139, 84, "PaleGreen4"),
        TestColor(0, 255, 127, "SpringGreen1"),
        TestColor(0, 238, 118, "SpringGreen2"),
        TestColor(0, 205, 102, "SpringGreen3"),
        TestColor(0, 139, 69, "SpringGreen4"),
        TestColor(0, 255, 0, "green1"),
        TestColor(0, 238, 0, "green2"),
        TestColor(0, 205, 0, "green3"),
        TestColor(0, 139, 0, "green4"),
        TestColor(127, 255, 0, "chartreuse1"),
        TestColor(118, 238, 0, "chartreuse2"),
        TestColor(102, 205, 0, "chartreuse3"),
        TestColor(69, 139, 0, "chartreuse4"),
        TestColor(192, 255, 62, "OliveDrab1"),
        TestColor(179, 238, 58, "OliveDrab2"),
        TestColor(154, 205, 50, "OliveDrab3"),
        TestColor(105, 139, 34, "OliveDrab4"),
        TestColor(202, 255, 112, "DarkOliveGreen1"),
        TestColor(188, 238, 104, "DarkOliveGreen2"),
        TestColor(162, 205, 90, "DarkOliveGreen3"),
        TestColor(110, 139, 61, "DarkOliveGreen4"),
        TestColor(255, 246, 143, "khaki1"),
        TestColor(238, 230, 133, "khaki2"),
        TestColor(205, 198, 115, "khaki3"),
        TestColor(139, 134, 78, "khaki4"),
        TestColor(255, 236, 139, "LightGoldenrod1"),
        TestColor(238, 220, 130, "LightGoldenrod2"),
        TestColor(205, 190, 112, "LightGoldenrod3"),
        TestColor(139, 129, 76, "LightGoldenrod4"),
        TestColor(255, 255, 224, "LightYellow1"),
        TestColor(238, 238, 209, "LightYellow2"),
        TestColor(205, 205, 180, "LightYellow3"),
        TestColor(139, 139, 122, "LightYellow4"),
        TestColor(255, 255, 0, "yellow1"),
        TestColor(238, 238, 0, "yellow2"),
        TestColor(205, 205, 0, "yellow3"),
        TestColor(139, 139, 0, "yellow4"),
        TestColor(255, 215, 0, "gold1"),
        TestColor(238, 201, 0, "gold2"),
        TestColor(205, 173, 0, "gold3"),
        TestColor(139, 117, 0, "gold4"),
        TestColor(255, 193, 37, "goldenrod1"),
        TestColor(238, 180, 34, "goldenrod2"),
        TestColor(205, 155, 29, "goldenrod3"),
        TestColor(139, 105, 20, "goldenrod4"),
        TestColor(255, 185, 15, "DarkGoldenrod1"),
        TestColor(238, 173, 14, "DarkGoldenrod2"),
        TestColor(205, 149, 12, "DarkGoldenrod3"),
        TestColor(139, 101, 8, "DarkGoldenrod4"),
        TestColor(255, 193, 193, "RosyBrown1"),
        TestColor(238, 180, 180, "RosyBrown2"),
        TestColor(205, 155, 155, "RosyBrown3"),
        TestColor(139, 105, 105, "RosyBrown4"),
        TestColor(255, 106, 106, "IndianRed1"),
        TestColor(238, 99, 99, "IndianRed2"),
        TestColor(205, 85, 85, "IndianRed3"),
        TestColor(139, 58, 58, "IndianRed4"),
        TestColor(255, 130, 71, "sienna1"),
        TestColor(238, 121, 66, "sienna2"),
        TestColor(205, 104, 57, "sienna3"),
        TestColor(139, 71, 38, "sienna4"),
        TestColor(255, 211, 155, "burlywood1"),
        TestColor(238, 197, 145, "burlywood2"),
        TestColor(205, 170, 125, "burlywood3"),
        TestColor(139, 115, 85, "burlywood4"),
        TestColor(255, 231, 186, "wheat1"),
        TestColor(238, 216, 174, "wheat2"),
        TestColor(205, 186, 150, "wheat3"),
        TestColor(139, 126, 102, "wheat4"),
        TestColor(255, 165, 79, "tan1"),
        TestColor(238, 154, 73, "tan2"),
        TestColor(205, 133, 63, "tan3"),
        TestColor(139, 90, 43, "tan4"),
        TestColor(255, 127, 36, "chocolate1"),
        TestColor(238, 118, 33, "chocolate2"),
        TestColor(205, 102, 29, "chocolate3"),
        TestColor(139, 69, 19, "chocolate4"),
        TestColor(255, 48, 48, "firebrick1"),
        TestColor(238, 44, 44, "firebrick2"),
        TestColor(205, 38, 38, "firebrick3"),
        TestColor(139, 26, 26, "firebrick4"),
        TestColor(255, 64, 64, "brown1"),
        TestColor(238, 59, 59, "brown2"),
        TestColor(205, 51, 51, "brown3"),
        TestColor(139, 35, 35, "brown4"),
        TestColor(255, 140, 105, "salmon1"),
        TestColor(238, 130, 98, "salmon2"),
        TestColor(205, 112, 84, "salmon3"),
        TestColor(139, 76, 57, "salmon4"),
        TestColor(255, 160, 122, "LightSalmon1"),
        TestColor(238, 149, 114, "LightSalmon2"),
        TestColor(205, 129, 98, "LightSalmon3"),
        TestColor(139, 87, 66, "LightSalmon4"),
        TestColor(255, 165, 0, "orange1"),
        TestColor(238, 154, 0, "orange2"),
        TestColor(205, 133, 0, "orange3"),
        TestColor(139, 90, 0, "orange4"),
        TestColor(255, 127, 0, "DarkOrange1"),
        TestColor(238, 118, 0, "DarkOrange2"),
        TestColor(205, 102, 0, "DarkOrange3"),
        TestColor(139, 69, 0, "DarkOrange4"),
        TestColor(255, 114, 86, "coral1"),
        TestColor(238, 106, 80, "coral2"),
        TestColor(205, 91, 69, "coral3"),
        TestColor(139, 62, 47, "coral4"),
        TestColor(255, 99, 71, "tomato1"),
        TestColor(238, 92, 66, "tomato2"),
        TestColor(205, 79, 57, "tomato3"),
        TestColor(139, 54, 38, "tomato4"),
        TestColor(255, 69, 0, "OrangeRed1"),
        TestColor(238, 64, 0, "OrangeRed2"),
        TestColor(205, 55, 0, "OrangeRed3"),
        TestColor(139, 37, 0, "OrangeRed4"),
        TestColor(255, 0, 0, "red1"),
        TestColor(238, 0, 0, "red2"),
        TestColor(205, 0, 0, "red3"),
        TestColor(139, 0, 0, "red4"),
        TestColor(255, 20, 147, "DeepPink1"),
        TestColor(238, 18, 137, "DeepPink2"),
        TestColor(205, 16, 118, "DeepPink3"),
        TestColor(139, 10, 80, "DeepPink4"),
        TestColor(255, 110, 180, "HotPink1"),
        TestColor(238, 106, 167, "HotPink2"),
        TestColor(205, 96, 144, "HotPink3"),
        TestColor(139, 58, 98, "HotPink4"),
        TestColor(255, 181, 197, "pink1"),
        TestColor(238, 169, 184, "pink2"),
        TestColor(205, 145, 158, "pink3"),
        TestColor(139, 99, 108, "pink4"),
        TestColor(255, 174, 185, "LightPink1"),
        TestColor(238, 162, 173, "LightPink2"),
        TestColor(205, 140, 149, "LightPink3"),
        TestColor(139, 95, 101, "LightPink4"),
        TestColor(255, 130, 171, "PaleVioletRed1"),
        TestColor(238, 121, 159, "PaleVioletRed2"),
        TestColor(205, 104, 137, "PaleVioletRed3"),
        TestColor(139, 71, 93, "PaleVioletRed4"),
        TestColor(255, 52, 179, "maroon1"),
        TestColor(238, 48, 167, "maroon2"),
        TestColor(205, 41, 144, "maroon3"),
        TestColor(139, 28, 98, "maroon4"),
        TestColor(255, 62, 150, "VioletRed1"),
        TestColor(238, 58, 140, "VioletRed2"),
        TestColor(205, 50, 120, "VioletRed3"),
        TestColor(139, 34, 82, "VioletRed4"),
        TestColor(255, 0, 255, "magenta1"),
        TestColor(238, 0, 238, "magenta2"),
        TestColor(205, 0, 205, "magenta3"),
        TestColor(139, 0, 139, "magenta4"),
        TestColor(255, 131, 250, "orchid1"),
        TestColor(238, 122, 233, "orchid2"),
        TestColor(205, 105, 201, "orchid3"),
        TestColor(139, 71, 137, "orchid4"),
        TestColor(255, 187, 255, "plum1"),
        TestColor(238, 174, 238, "plum2"),
        TestColor(205, 150, 205, "plum3"),
        TestColor(139, 102, 139, "plum4"),
        TestColor(224, 102, 255, "MediumOrchid1"),
        TestColor(209, 95, 238, "MediumOrchid2"),
        TestColor(180, 82, 205, "MediumOrchid3"),
        TestColor(122, 55, 139, "MediumOrchid4"),
        TestColor(191, 62, 255, "DarkOrchid1"),
        TestColor(178, 58, 238, "DarkOrchid2"),
        TestColor(154, 50, 205, "DarkOrchid3"),
        TestColor(104, 34, 139, "DarkOrchid4"),
        TestColor(155, 48, 255, "purple1"),
        TestColor(145, 44, 238, "purple2"),
        TestColor(125, 38, 205, "purple3"),
        TestColor(85, 26, 139, "purple4"),
        TestColor(171, 130, 255, "MediumPurple1"),
        TestColor(159, 121, 238, "MediumPurple2"),
        TestColor(137, 104, 205, "MediumPurple3"),
        TestColor(93, 71, 139, "MediumPurple4"),
        TestColor(255, 225, 255, "thistle1"),
        TestColor(238, 210, 238, "thistle2"),
        TestColor(205, 181, 205, "thistle3"),
        TestColor(139, 123, 139, "thistle4"),
        TestColor(0, 0, 0, "gray0"),
        TestColor(0, 0, 0, "grey0"),
        TestColor(3, 3, 3, "gray1"),
        TestColor(3, 3, 3, "grey1"),
        TestColor(5, 5, 5, "gray2"),
        TestColor(5, 5, 5, "grey2"),
        TestColor(8, 8, 8, "gray3"),
        TestColor(8, 8, 8, "grey3"),
        TestColor(10, 10, 10, "gray4"),
        TestColor(10, 10, 10, "grey4"),
        TestColor(13, 13, 13, "gray5"),
        TestColor(13, 13, 13, "grey5"),
        TestColor(15, 15, 15, "gray6"),
        TestColor(15, 15, 15, "grey6"),
        TestColor(18, 18, 18, "gray7"),
        TestColor(18, 18, 18, "grey7"),
        TestColor(20, 20, 20, "gray8"),
        TestColor(20, 20, 20, "grey8"),
        TestColor(23, 23, 23, "gray9"),
        TestColor(23, 23, 23, "grey9"),
        TestColor(26, 26, 26, "gray10"),
        TestColor(26, 26, 26, "grey10"),
        TestColor(28, 28, 28, "gray11"),
        TestColor(28, 28, 28, "grey11"),
        TestColor(31, 31, 31, "gray12"),
        TestColor(31, 31, 31, "grey12"),
        TestColor(33, 33, 33, "gray13"),
        TestColor(33, 33, 33, "grey13"),
        TestColor(36, 36, 36, "gray14"),
        TestColor(36, 36, 36, "grey14"),
        TestColor(38, 38, 38, "gray15"),
        TestColor(38, 38, 38, "grey15"),
        TestColor(41, 41, 41, "gray16"),
        TestColor(41, 41, 41, "grey16"),
        TestColor(43, 43, 43, "gray17"),
        TestColor(43, 43, 43, "grey17"),
        TestColor(46, 46, 46, "gray18"),
        TestColor(46, 46, 46, "grey18"),
        TestColor(48, 48, 48, "gray19"),
        TestColor(48, 48, 48, "grey19"),
        TestColor(51, 51, 51, "gray20"),
        TestColor(51, 51, 51, "grey20"),
        TestColor(54, 54, 54, "gray21"),
        TestColor(54, 54, 54, "grey21"),
        TestColor(56, 56, 56, "gray22"),
        TestColor(56, 56, 56, "grey22"),
        TestColor(59, 59, 59, "gray23"),
        TestColor(59, 59, 59, "grey23"),
        TestColor(61, 61, 61, "gray24"),
        TestColor(61, 61, 61, "grey24"),
        TestColor(64, 64, 64, "gray25"),
        TestColor(64, 64, 64, "grey25"),
        TestColor(66, 66, 66, "gray26"),
        TestColor(66, 66, 66, "grey26"),
        TestColor(69, 69, 69, "gray27"),
        TestColor(69, 69, 69, "grey27"),
        TestColor(71, 71, 71, "gray28"),
        TestColor(71, 71, 71, "grey28"),
        TestColor(74, 74, 74, "gray29"),
        TestColor(74, 74, 74, "grey29"),
        TestColor(77, 77, 77, "gray30"),
        TestColor(77, 77, 77, "grey30"),
        TestColor(79, 79, 79, "gray31"),
        TestColor(79, 79, 79, "grey31"),
        TestColor(82, 82, 82, "gray32"),
        TestColor(82, 82, 82, "grey32"),
        TestColor(84, 84, 84, "gray33"),
        TestColor(84, 84, 84, "grey33"),
        TestColor(87, 87, 87, "gray34"),
        TestColor(87, 87, 87, "grey34"),
        TestColor(89, 89, 89, "gray35"),
        TestColor(89, 89, 89, "grey35"),
        TestColor(92, 92, 92, "gray36"),
        TestColor(92, 92, 92, "grey36"),
        TestColor(94, 94, 94, "gray37"),
        TestColor(94, 94, 94, "grey37"),
        TestColor(97, 97, 97, "gray38"),
        TestColor(97, 97, 97, "grey38"),
        TestColor(99, 99, 99, "gray39"),
        TestColor(99, 99, 99, "grey39"),
        TestColor(102, 102, 102, "gray40"),
        TestColor(102, 102, 102, "grey40"),
        TestColor(105, 105, 105, "gray41"),
        TestColor(105, 105, 105, "grey41"),
        TestColor(107, 107, 107, "gray42"),
        TestColor(107, 107, 107, "grey42"),
        TestColor(110, 110, 110, "gray43"),
        TestColor(110, 110, 110, "grey43"),
        TestColor(112, 112, 112, "gray44"),
        TestColor(112, 112, 112, "grey44"),
        TestColor(115, 115, 115, "gray45"),
        TestColor(115, 115, 115, "grey45"),
        TestColor(117, 117, 117, "gray46"),
        TestColor(117, 117, 117, "grey46"),
        TestColor(120, 120, 120, "gray47"),
        TestColor(120, 120, 120, "grey47"),
        TestColor(122, 122, 122, "gray48"),
        TestColor(122, 122, 122, "grey48"),
        TestColor(125, 125, 125, "gray49"),
        TestColor(125, 125, 125, "grey49"),
        TestColor(127, 127, 127, "gray50"),
        TestColor(127, 127, 127, "grey50"),
        TestColor(130, 130, 130, "gray51"),
        TestColor(130, 130, 130, "grey51"),
        TestColor(133, 133, 133, "gray52"),
        TestColor(133, 133, 133, "grey52"),
        TestColor(135, 135, 135, "gray53"),
        TestColor(135, 135, 135, "grey53"),
        TestColor(138, 138, 138, "gray54"),
        TestColor(138, 138, 138, "grey54"),
        TestColor(140, 140, 140, "gray55"),
        TestColor(140, 140, 140, "grey55"),
        TestColor(143, 143, 143, "gray56"),
        TestColor(143, 143, 143, "grey56"),
        TestColor(145, 145, 145, "gray57"),
        TestColor(145, 145, 145, "grey57"),
        TestColor(148, 148, 148, "gray58"),
        TestColor(148, 148, 148, "grey58"),
        TestColor(150, 150, 150, "gray59"),
        TestColor(150, 150, 150, "grey59"),
        TestColor(153, 153, 153, "gray60"),
        TestColor(153, 153, 153, "grey60"),
        TestColor(156, 156, 156, "gray61"),
        TestColor(156, 156, 156, "grey61"),
        TestColor(158, 158, 158, "gray62"),
        TestColor(158, 158, 158, "grey62"),
        TestColor(161, 161, 161, "gray63"),
        TestColor(161, 161, 161, "grey63"),
        TestColor(163, 163, 163, "gray64"),
        TestColor(163, 163, 163, "grey64"),
        TestColor(166, 166, 166, "gray65"),
        TestColor(166, 166, 166, "grey65"),
        TestColor(168, 168, 168, "gray66"),
        TestColor(168, 168, 168, "grey66"),
        TestColor(171, 171, 171, "gray67"),
        TestColor(171, 171, 171, "grey67"),
        TestColor(173, 173, 173, "gray68"),
        TestColor(173, 173, 173, "grey68"),
        TestColor(176, 176, 176, "gray69"),
        TestColor(176, 176, 176, "grey69"),
        TestColor(179, 179, 179, "gray70"),
        TestColor(179, 179, 179, "grey70"),
        TestColor(181, 181, 181, "gray71"),
        TestColor(181, 181, 181, "grey71"),
        TestColor(184, 184, 184, "gray72"),
        TestColor(184, 184, 184, "grey72"),
        TestColor(186, 186, 186, "gray73"),
        TestColor(186, 186, 186, "grey73"),
        TestColor(189, 189, 189, "gray74"),
        TestColor(189, 189, 189, "grey74"),
        TestColor(191, 191, 191, "gray75"),
        TestColor(191, 191, 191, "grey75"),
        TestColor(194, 194, 194, "gray76"),
        TestColor(194, 194, 194, "grey76"),
        TestColor(196, 196, 196, "gray77"),
        TestColor(196, 196, 196, "grey77"),
        TestColor(199, 199, 199, "gray78"),
        TestColor(199, 199, 199, "grey78"),
        TestColor(201, 201, 201, "gray79"),
        TestColor(201, 201, 201, "grey79"),
        TestColor(204, 204, 204, "gray80"),
        TestColor(204, 204, 204, "grey80"),
        TestColor(207, 207, 207, "gray81"),
        TestColor(207, 207, 207, "grey81"),
        TestColor(209, 209, 209, "gray82"),
        TestColor(209, 209, 209, "grey82"),
        TestColor(212, 212, 212, "gray83"),
        TestColor(212, 212, 212, "grey83"),
        TestColor(214, 214, 214, "gray84"),
        TestColor(214, 214, 214, "grey84"),
        TestColor(217, 217, 217, "gray85"),
        TestColor(217, 217, 217, "grey85"),
        TestColor(219, 219, 219, "gray86"),
        TestColor(219, 219, 219, "grey86"),
        TestColor(222, 222, 222, "gray87"),
        TestColor(222, 222, 222, "grey87"),
        TestColor(224, 224, 224, "gray88"),
        TestColor(224, 224, 224, "grey88"),
        TestColor(227, 227, 227, "gray89"),
        TestColor(227, 227, 227, "grey89"),
        TestColor(229, 229, 229, "gray90"),
        TestColor(229, 229, 229, "grey90"),
        TestColor(232, 232, 232, "gray91"),
        TestColor(232, 232, 232, "grey91"),
        TestColor(235, 235, 235, "gray92"),
        TestColor(235, 235, 235, "grey92"),
        TestColor(237, 237, 237, "gray93"),
        TestColor(237, 237, 237, "grey93"),
        TestColor(240, 240, 240, "gray94"),
        TestColor(240, 240, 240, "grey94"),
        TestColor(242, 242, 242, "gray95"),
        TestColor(242, 242, 242, "grey95"),
        TestColor(245, 245, 245, "gray96"),
        TestColor(245, 245, 245, "grey96"),
        TestColor(247, 247, 247, "gray97"),
        TestColor(247, 247, 247, "grey97"),
        TestColor(250, 250, 250, "gray98"),
        TestColor(250, 250, 250, "grey98"),
        TestColor(252, 252, 252, "gray99"),
        TestColor(252, 252, 252, "grey99"),
        TestColor(255, 255, 255, "gray100"),
        TestColor(255, 255, 255, "grey100"),
        TestColor(169, 169, 169, "DarkGrey"),
        TestColor(169, 169, 169, "DarkGray"),
        TestColor(0, 0, 139, "DarkBlue"),
        TestColor(0, 139, 139, "DarkCyan"),
        TestColor(139, 0, 139, "DarkMagenta"),
        TestColor(139, 0, 0, "DarkRed"),
        TestColor(144, 238, 144, "LightGreen")
    };

    const size_t SIZE_OF_TABLE_OF_TEST_COLORS = sizeof(TABLE_OF_TEST_COLORS)/sizeof(TestColor);

    for (size_t i=0; i<SIZE_OF_TABLE_OF_TEST_COLORS; ++i)
    {
        const TestColor& TEST_COLOR(TABLE_OF_TEST_COLORS[i]);

        const PdfColor COLOR_FROM_NAME(PdfColor::FromString(TEST_COLOR.getColorName()));
        const PdfColor EXPECTED_COLOR(TEST_COLOR.getR(), TEST_COLOR.getG(), TEST_COLOR.getB());

        if (PdfColor() == COLOR_FROM_NAME)
        {
#ifdef DEBUG_INFO
            std::cout << "Color " << TEST_COLOR.getColorName() << " is not supported" << std::endl;
#endif
        }
        else
        {
            bool result = EXPECTED_COLOR == COLOR_FROM_NAME;
            ASSERT_TRUE(result);
        }
    }
}

void ColorTest::testColorGreyConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testColorGreyConstructor" << std::endl;
#endif

    const double GREY_VALUE = 0.123;
    PdfColorGray color(GREY_VALUE);

    ASSERT_TRUE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceGray);

    ASSERT_EQ(color.GetGrayScale(), GREY_VALUE);
}

void ColorTest::testColorRGBConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testColorRGBConstructor" << std::endl;
#endif

    const double R_VALUE = 0.023;
    const double G_VALUE = 0.345;
    const double B_VALUE = 0.678;
    PdfColorRGB color(R_VALUE, G_VALUE, B_VALUE);

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_TRUE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceRGB);

    ASSERT_EQ(color.GetRed(), R_VALUE);
    ASSERT_EQ(color.GetGreen(), G_VALUE);
    ASSERT_EQ(color.GetBlue(), B_VALUE);
}

void ColorTest::testColorCMYKConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testColorCMYKConstructor" << std::endl;
#endif

    const double C_VALUE = 0.1;
    const double M_VALUE = 0.2;
    const double Y_VALUE = 0.3;
    const double B_VALUE = 0.4;
    PdfColorCMYK color(C_VALUE, M_VALUE, Y_VALUE, B_VALUE);

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_TRUE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_DeviceCMYK);

    ASSERT_EQ(color.GetCyan(), C_VALUE);
    ASSERT_EQ(color.GetMagenta(), M_VALUE);
    ASSERT_EQ(color.GetYellow(), Y_VALUE);
    ASSERT_EQ(color.GetBlack(), B_VALUE);
}

void ColorTest::testColorSeparationAllConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testColorSeparationAllConstructor" << std::endl;
#endif

    PdfColorSeparationAll color;

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_TRUE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_Separation);
    ASSERT_EQ(color.GetAlternateColorSpace(), ePdfColorSpace_DeviceCMYK);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGrayScale(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetRed(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGreen(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlue(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_EQ(color.GetCyan(), 1.0);
    ASSERT_EQ(color.GetMagenta(), 1.0);
    ASSERT_EQ(color.GetYellow(), 1.0);
    ASSERT_EQ(color.GetBlack(), 1.0);

    ASSERT_EQ(color.GetName(), std::string("All"));
    ASSERT_EQ(color.GetDensity(), 1.0);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieL(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieA(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieB(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_TRUE(PdfColor(0.0, 0.0, 0.0) == color.ConvertToGrayScale());
    ASSERT_TRUE(PdfColor(0.0, 0.0, 0.0) == color.ConvertToRGB());

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToCMYK(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    const PdfArray COLOR_ARRAY = color.ToArray();
    ASSERT_TRUE(1 == COLOR_ARRAY.GetSize());
    ASSERT_TRUE(PdfObject(1.0) == COLOR_ARRAY[0]);
}

void ColorTest::testColorSeparationNoneConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testColorSeparationNoneConstructor" << std::endl;
#endif

    PdfColorSeparationNone color;

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_TRUE(color.IsSeparation());
    ASSERT_FALSE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_Separation);
    ASSERT_EQ(color.GetAlternateColorSpace(), ePdfColorSpace_DeviceCMYK);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGrayScale(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetRed(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGreen(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlue(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_EQ(color.GetCyan(), 0.0);
    ASSERT_EQ(color.GetMagenta(), 0.0);
    ASSERT_EQ(color.GetYellow(), 0.0);
    ASSERT_EQ(color.GetBlack(), 0.0);

    ASSERT_EQ(color.GetName(), std::string("None"));
    ASSERT_EQ(color.GetDensity(), 0.0);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieL(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieA(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCieB(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_TRUE(PdfColor(1.0, 1.0, 1.0) == color.ConvertToGrayScale());
    ASSERT_TRUE(PdfColor(1.0, 1.0, 1.0) == color.ConvertToRGB());

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToCMYK(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    const PdfArray COLOR_ARRAY = color.ToArray();
    ASSERT_TRUE(1 == COLOR_ARRAY.GetSize());
    ASSERT_TRUE(PdfObject(0.0) == COLOR_ARRAY[0]);
}

void ColorTest::testColorSeparationConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testColorSeparationConstructor" << std::endl;
#endif

    { //alternate color is Greyscale
        const PdfColorGray ALTERNATE_COLOR(0.1234);
        const double DENSITY = 0.523456;
        const std::string NAME("Hello");
        PdfColorSeparation color("Hello", DENSITY, ALTERNATE_COLOR);

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_TRUE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_Separation);
        ASSERT_EQ(color.GetAlternateColorSpace(), ePdfColorSpace_DeviceGray);

        ASSERT_EQ(ALTERNATE_COLOR.GetGrayScale(), color.GetGrayScale());

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetRed(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetGreen(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetBlue(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCyan(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetMagenta(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetYellow(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetBlack(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieL(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieA(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieB(), 
            PdfError, 
            ePdfError_InternalLogic);

        ASSERT_EQ(color.GetName(), NAME);
        ASSERT_EQ(color.GetDensity(), DENSITY);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToGrayScale(), 
            PdfError, 
            ePdfError_NotImplemented);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToRGB(), 
            PdfError, 
            ePdfError_NotImplemented);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToCMYK(), 
            PdfError, 
            ePdfError_CannotConvertColor);

        const PdfArray COLOR_ARRAY = color.ToArray();
        ASSERT_TRUE(1 == COLOR_ARRAY.GetSize());
        ASSERT_TRUE(PdfObject(0.0) == COLOR_ARRAY[0]);
    }

    { //alternate color is RGB
        const double R_VALUE = 0.023;
        const double G_VALUE = 0.345;
        const double B_VALUE = 0.678;
        const PdfColor ALTERNATE_COLOR(R_VALUE, G_VALUE, B_VALUE);
        const double DENSITY = 0.523456;
        const std::string NAME("Hello");
        PdfColorSeparation color("Hello", DENSITY, ALTERNATE_COLOR);

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_TRUE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_Separation);
        ASSERT_EQ(color.GetAlternateColorSpace(), ePdfColorSpace_DeviceRGB);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetGrayScale(), 
            PdfError, 
            ePdfError_InternalLogic);

        ASSERT_EQ(ALTERNATE_COLOR.GetRed(), color.GetRed());
        ASSERT_EQ(ALTERNATE_COLOR.GetGreen(), color.GetGreen());
        ASSERT_EQ(ALTERNATE_COLOR.GetBlue(), color.GetBlue());

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCyan(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetMagenta(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetYellow(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetBlack(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieL(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieA(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieB(), 
            PdfError, 
            ePdfError_InternalLogic);

        ASSERT_EQ(color.GetName(), NAME);
        ASSERT_EQ(color.GetDensity(), DENSITY);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToGrayScale(), 
            PdfError, 
            ePdfError_NotImplemented);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToRGB(), 
            PdfError, 
            ePdfError_NotImplemented);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToCMYK(), 
            PdfError, 
            ePdfError_CannotConvertColor);

        const PdfArray COLOR_ARRAY = color.ToArray();
        ASSERT_TRUE(1 == COLOR_ARRAY.GetSize());
        ASSERT_TRUE(PdfObject(0.0) == COLOR_ARRAY[0]);
    }

    { //alternate color is CMYK
        const double C_VALUE = 0.023;
        const double M_VALUE = 0.345;
        const double Y_VALUE = 0.678;
        const double K_VALUE = 0.18;
        const PdfColor ALTERNATE_COLOR(C_VALUE, M_VALUE, Y_VALUE, K_VALUE);
        const double DENSITY = 0.123456;
        const std::string NAME("Hello");
        PdfColorSeparation color("Hello", DENSITY, ALTERNATE_COLOR);

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_TRUE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_Separation);
        ASSERT_EQ(color.GetAlternateColorSpace(), ePdfColorSpace_DeviceCMYK);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetGrayScale(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetRed(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetGreen(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetBlue(), 
            PdfError, 
            ePdfError_InternalLogic);

        ASSERT_EQ(ALTERNATE_COLOR.GetCyan(), color.GetCyan());
        ASSERT_EQ(ALTERNATE_COLOR.GetMagenta(), color.GetMagenta());
        ASSERT_EQ(ALTERNATE_COLOR.GetYellow(), color.GetYellow());
        ASSERT_EQ(ALTERNATE_COLOR.GetBlack(), color.GetBlack());

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieL(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieA(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCieB(), 
            PdfError, 
            ePdfError_InternalLogic);

        ASSERT_EQ(color.GetName(), NAME);
        ASSERT_EQ(color.GetDensity(), DENSITY);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToCMYK(), 
            PdfError, 
            ePdfError_CannotConvertColor);

        const PdfArray COLOR_ARRAY = color.ToArray();
        ASSERT_TRUE(1 == COLOR_ARRAY.GetSize());
        ASSERT_TRUE(PdfObject(0.0) == COLOR_ARRAY[0]);
    }

    { //alternate color is CieLab
        const double dCieL = 0.023;
        const double dCieA = 0.345;
        const double dCieB = 0.678;
        const PdfColorCieLab ALTERNATE_COLOR(dCieL, dCieA, dCieB);
        const double DENSITY = 0.523456;
        const std::string NAME("Hello");
        PdfColorSeparation color("Hello", DENSITY, ALTERNATE_COLOR);

        ASSERT_FALSE(color.IsGrayScale());
        ASSERT_FALSE(color.IsRGB());
        ASSERT_FALSE(color.IsCMYK());
        ASSERT_TRUE(color.IsSeparation());
        ASSERT_FALSE(color.IsCieLab());
        ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_Separation);
        ASSERT_EQ(color.GetAlternateColorSpace(), ePdfColorSpace_CieLab);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetGrayScale(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetRed(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetGreen(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetBlue(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetCyan(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetMagenta(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetYellow(), 
            PdfError, 
            ePdfError_InternalLogic);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.GetBlack(), 
            PdfError, 
            ePdfError_InternalLogic);

        ASSERT_EQ(ALTERNATE_COLOR.GetCieL(), color.GetCieL());
        ASSERT_EQ(ALTERNATE_COLOR.GetCieA(), color.GetCieA());
        ASSERT_EQ(ALTERNATE_COLOR.GetCieB(), color.GetCieB());

        ASSERT_EQ(color.GetName(), NAME);
        ASSERT_EQ(color.GetDensity(), DENSITY);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToGrayScale(), 
            PdfError, 
            ePdfError_NotImplemented);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToRGB(), 
            PdfError, 
            ePdfError_NotImplemented);

        CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
            color.ConvertToCMYK(), 
            PdfError, 
            ePdfError_CannotConvertColor);

        const PdfArray COLOR_ARRAY = color.ToArray();
        ASSERT_TRUE(1 == COLOR_ARRAY.GetSize());
        ASSERT_TRUE(PdfObject(0.0) == COLOR_ARRAY[0]);
    }
}

void ColorTest::testColorCieLabConstructor() 
{
#ifdef DEBUG_INFO
    std::cout << "testColorCieLabConstructor" << std::endl;
#endif

    const double dCieL = 0.023;
    const double dCieA = 0.345;
    const double dCieB = 0.678;
    PdfColorCieLab color(dCieL, dCieA, dCieB);

    ASSERT_FALSE(color.IsGrayScale());
    ASSERT_FALSE(color.IsRGB());
    ASSERT_FALSE(color.IsCMYK());
    ASSERT_FALSE(color.IsSeparation());
    ASSERT_TRUE(color.IsCieLab());
    ASSERT_EQ(color.GetColorSpace(), ePdfColorSpace_CieLab);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetAlternateColorSpace(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGrayScale(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetRed(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetGreen(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlue(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetCyan(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetMagenta(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetYellow(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetBlack(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetName(), 
        PdfError, 
        ePdfError_InternalLogic);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.GetDensity(), 
        PdfError, 
        ePdfError_InternalLogic);

    ASSERT_EQ(dCieL, color.GetCieL());
    ASSERT_EQ(dCieA, color.GetCieA());
    ASSERT_EQ(dCieB, color.GetCieB());

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToGrayScale(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToRGB(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    CPPUNIT_ASSERT_THROW_WITH_ERROR_TYPE( 
        color.ConvertToCMYK(), 
        PdfError, 
        ePdfError_CannotConvertColor);

    const PdfArray COLOR_ARRAY = color.ToArray();
    ASSERT_TRUE(3 == COLOR_ARRAY.GetSize());
    ASSERT_TRUE(PdfObject(dCieL) == COLOR_ARRAY[0]);
    ASSERT_TRUE(PdfObject(dCieA) == COLOR_ARRAY[1]);
    ASSERT_TRUE(PdfObject(dCieB) == COLOR_ARRAY[2]);
}

void ColorTest::testRGBtoCMYKConversions() 
{
#ifdef DEBUG_INFO
    std::cout << "testRGBtoCMYKConversions" << std::endl;
#endif

    typedef std::pair<PdfColor, PdfColor> TPairOfColors;
    typedef std::map<std::string, TPairOfColors> TMapOfColors;

    TMapOfColors colorTable;
    colorTable["red"]    = TPairOfColors(PdfColor(1.0, 0.0, 0.0), PdfColor(0.0, 1.0, 1.0, 0.0));
    colorTable["green"]  = TPairOfColors(PdfColor(0.0, 1.0, 0.0), PdfColor(1.0, 0.0, 1.0, 0.0));
    colorTable["blue"]   = TPairOfColors(PdfColor(0.0, 0.0, 1.0), PdfColor(1.0, 1.0, 0.0, 0.0));
    colorTable["white"]  = TPairOfColors(PdfColor(1.0, 1.0, 1.0), PdfColor(0.0, 0.0, 0.0, 0.0));
    colorTable["black"]  = TPairOfColors(PdfColor(0.0, 0.0, 0.0), PdfColor(0.0, 0.0, 0.0, 1.0));
    colorTable["cyan"]   = TPairOfColors(PdfColor(0.0, 1.0, 1.0), PdfColor(1.0, 0.0, 0.0, 0.0));
    colorTable["magenta"]= TPairOfColors(PdfColor(1.0, 0.0, 1.0), PdfColor(0.0, 1.0, 0.0, 0.0));
    colorTable["yellow"] = TPairOfColors(PdfColor(1.0, 1.0, 0.0), PdfColor(0.0, 0.0, 1.0, 0.0));

    for(TMapOfColors::const_iterator iter(colorTable.begin()), iterEnd(colorTable.end());
        iter != iterEnd;
        ++iter)
    {
        const std::string COLOR_NAME(iter->first);
        PdfColor namedColor(PdfColor::FromString(COLOR_NAME.c_str()));
        PdfColor rgbColor(iter->second.first);
        PdfColor cmykColor(iter->second.second);

        ASSERT_TRUE(namedColor.ConvertToRGB() == rgbColor);
        ASSERT_TRUE(rgbColor.ConvertToCMYK() == cmykColor);
        ASSERT_TRUE(rgbColor == cmykColor.ConvertToRGB());
    }
}
