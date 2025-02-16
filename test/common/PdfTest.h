/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_TEST_H
#define PDF_TEST_H

#include "catch.hpp"

#include <PdfTestConfig.h>

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/podofo.h>

#include <sstream>
#include <filesystem>

/** Check if a suitable error message is returned
 * Asserts that the given expression throws an exception of the specified type
 */
#define ASSERT_THROW_WITH_ERROR_CODE(expression, errorCode)                     \
    do                                                                          \
    {                                                                           \
        bool cpputCorrectExceptionThrown_ = false;                              \
        std::ostringstream stream;                                              \
        stream << "expected exception not thrown" << std::endl;                 \
        stream << "Expected: " #errorCode << std::endl;                         \
                                                                                \
        try                                                                     \
        {                                                                       \
            expression;                                                         \
        }                                                                       \
        catch (const PdfError &e)                                               \
        {                                                                       \
            if (e.GetCode() == errorCode)                                       \
            {                                                                   \
                cpputCorrectExceptionThrown_ = true;                            \
            }                                                                   \
            else                                                                \
            {                                                                   \
                stream << "Error type mismatch. Actual: " << #errorCode         \
                    << std::endl;                                               \
                stream << "What: " << e.GetName();                              \
            }                                                                   \
        }                                                                       \
        catch (const std::exception &e)                                         \
        {                                                                       \
            stream << "Actual std::exception or derived" << std::endl;          \
            stream << "What: " << e.what() << std::endl;                        \
        }                                                                       \
        catch (...)                                                             \
        {                                                                       \
            stream << "Actual exception unknown." << std::endl;                 \
        }                                                                       \
                                                                                \
        if (cpputCorrectExceptionThrown_)                                       \
           break;                                                               \
                                                                                \
        FAIL(stream.str());                                                     \
    } while (false);


#define ASSERT_EQUAL(expected, actual) TestUtils::AssertEqual(expected, actual)

namespace utls
{
    void CombinePaths(std::filesystem::path& path, std::initializer_list<std::string_view> paths);

    template<typename ... Ts>
    std::filesystem::path CombinePaths(const std::string_view& path1, const std::string_view& path2,
        Ts&&... paths)
    {
        auto ret = std::filesystem::u8path(path1);
        ret /= std::filesystem::u8path(path2);
        CombinePaths(ret, { paths... });
        return ret;
    }
}

namespace PoDoFo
{
    namespace fs = std::filesystem;

    /**
     * This class contains utility methods that are
     * often needed when writing tests.
     */
    class TestUtils final
    {
    public:
        static constexpr double THRESHOLD = 0.001;

        static std::string GetTestOutputFilePath(const std::string_view& filename);
        static std::string GetTestInputFilePath(const std::string_view& filename);

        template<typename ... Ts>
        static std::string GetTestOutputFilePath(const std::string_view& path1,
            Ts&&... paths);

        template<typename ... Ts>
        static std::string GetTestInputFilePath(const std::string_view& path1,
            Ts&&... paths);

        static const fs::path& GetTestInputPath();
        static const fs::path& GetTestOutputPath();
        static void ReadTestInputFile(const std::string_view& filename, std::string& str);
        static void WriteTestOutputFile(const std::string_view& filename, const std::string_view& view);
        static void AssertEqual(double expected, double actual, double threshold = THRESHOLD);
        static void SaveFramePPM(charbuff& buffer, const void* data,
            PdfPixelFormat srcPixelFormat, unsigned width, unsigned height);
        static void SaveFramePPM(OutputStream& stream, const void* data,
            PdfPixelFormat srcPixelFormat, unsigned width, unsigned height);

        static bool IsBufferEqual(const bufferview& buffer, const std::string_view& filename);

        static bool AreFilesEqual(const std::string_view& filename1, const std::string_view& filename2);
    };

    template<typename ...Ts>
    inline std::string TestUtils::GetTestOutputFilePath(const std::string_view& path1, Ts && ...paths)
    {
        auto ret = GetTestOutputPath() / std::filesystem::u8path(path1);
        utls::CombinePaths(ret, { paths... });
        return ret.u8string();
    }

    template<typename ...Ts>
    inline std::string TestUtils::GetTestInputFilePath(const std::string_view& path1, Ts && ...paths)
    {
        auto ret = GetTestInputPath() / std::filesystem::u8path(path1);
        utls::CombinePaths(ret, { paths... });
        return ret.u8string();
    }
}

#endif // PDF_TEST_H
