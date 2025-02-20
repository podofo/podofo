/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ERROR_H
#define PDF_ERROR_H

#include <podofo/main/PdfDeclarations.h>

/** \file PdfError.h
 *  Error information and logging is implemented in this file.
 */

namespace PoDoFo {

/** Error Code enum values which are used in PdfError to describe the error.
 *
 *  If you add an error code to this enum, please also add it
 *  to PdfError::ErrorName() and PdfError::ErrorMessage().
 *
 *  \see PdfError
 */
enum class PdfErrorCode : uint8_t
{
    Unknown = 0,              ///< Unknown error
    InvalidHandle,            ///< Unexpected null pointer or invalid state
    FileNotFound,             ///< A file was not found or cannot be opened.
    IOError,	              ///< Tried to do something unsupported to an I/O device like seek a non-seekable input device
    UnexpectedEOF,            ///< End of file was reached but data was expected.
    OutOfMemory,              ///< Not enough memory to complete an operation.
    ValueOutOfRange,          ///< The specified memory is out of the allowed range.
    InternalLogic,            ///< An internal sanity check or assertion failed.
    InvalidEnumValue,         ///< An invalid enum value was specified.
    MaxRecursionReached,      ///< Reached maximum recursion depth
    ObjectNotFound,           ///< An object was requested but was not found
    BrokenFile,               ///< The file content is broken.

    InvalidPDF,               ///< The file is no PDF file.
    InvalidTrailer,           ///< The PDF file has no or an invalid trailer.
    InvalidNumber,            ///< A number was expected in the PDF file, but the read string is no number.
    InvalidEncoding,          ///< Invalid encoding information
    InvalidObject,            ///< Invalid object or none none was found.
    InvalidEOFToken,          ///< The PDF file has no or an invalid EOF marker.
    InvalidDataType,          ///< The passed datatype is invalid or was not recognized
    InvalidXRef,              ///< The XRef table is invalid
    InvalidXRefStream,        ///< A XRef stream is invalid
    InvalidPredictor,         ///< Invalid or unimplemented predictor
    InvalidStrokeStyle,       ///< Invalid stroke style during drawing
    InvalidStream,            ///< The stream is invalid
    InvalidKey,               ///< The specified key is invalid
    InvalidName,              ///< The specified Name is not valid in this context
    InvalidEncryptionDict,    ///< The encryption dictionary is invalid or misses a required key
    InvalidPassword,          ///< The password used to open the PDF file was invalid
    InvalidFontData,          ///< The font file is invalid
    InvalidContentStream,     ///< The content stream is invalid due to mismatched context pairing or other problems
    InvalidInput,             ///< Invalid input

    UnsupportedFilter,        ///< The requested filter is not yet implemented.
    UnsupportedFontFormat,    ///< This font format is not supported by PoDoFo.
    WrongDestinationType,     ///< The requested field is not available for the given destination type

    FlateError,               ///< Error in zlib
    FreeTypeError,            ///< Error in FreeType

    UnsupportedPixelFormat,   ///< This pixel format is not supported by PoDoFo.
    UnsupportedImageFormat,   ///< This image format is not supported by PoDoFo.
    CannotConvertColor,       ///< This color format cannot be converted.

    NotImplemented,           ///< This feature is currently not implemented.

    ItemAlreadyPresent,       ///< An item to be inserted is already in this container
    ChangeOnImmutable,        ///< Changing values on immutable objects is not allowed.

    XmpMetadataError,         ///< Error while creating or reading XMP metadata
    OpenSSLError,             ///< OpenSSL error
};

class PODOFO_API PdfErrorInfo final
{
public:
    PdfErrorInfo(std::string filepath, unsigned line, std::string info);
    PdfErrorInfo(const PdfErrorInfo& rhs) = default;
    PdfErrorInfo& operator=(const PdfErrorInfo& rhs) = default;

public:
    /** Get the file path of the error info relative to
     * source directory path
     */
    std::string_view GetFilePath() const;
    const std::string& GetFullFilePath() const { return m_FilePath; }
    inline unsigned GetLine() const { return m_Line; }
    inline const std::string& GetInformation() const { return m_Info; }

private:
    unsigned m_Line;
    std::string m_FilePath;
    std::string m_Info;
};

using PdErrorInfoStack = std::deque<PdfErrorInfo>;

// This is required to generate the documentation with Doxygen.
// Without this define doxygen thinks we have a class called PODOFO_EXCEPTION_API(PODOFO_API) ...
#define PODOFO_EXCEPTION_API_DOXYGEN PODOFO_EXCEPTION_API(PODOFO_API)

/** The error handling class of the PoDoFo library.
 *  If a method encounters an error,
 *  a PdfError object is thrown as a C++ exception.
 *
 *  This class does not inherit from std::exception.
 *
 *  This class also provides meaningful error descriptions
 *  for the error codes which are values of the enum PdfErrorCode,
 *  which are all codes PoDoFo uses (except the first and last one).
 */
class PODOFO_EXCEPTION_API_DOXYGEN PdfError final : public std::exception
{
    PODOFO_PRIVATE_FRIEND(void AddToCallStack(PdfError& err, std::string filepath, unsigned line, std::string information));

public:
    /** Create a PdfError object with a given error code.
     *  \param code the error code of this object
     *  \param filepath the file in which the error has occurred.
     *         Use the compiler macro __FILE__ to initialize the field.
     *  \param line the line in which the error has occurred.
     *         Use the compiler macro __LINE__ to initialize the field.
     *  \param information additional information on this error
     */
    PdfError(PdfErrorCode code, std::string filepath, unsigned line,
        std::string information = { });

    /** Copy constructor
     *  \param rhs copy the contents of rhs into this object
     */
    PdfError(const PdfError& rhs) = default;

    /** Assignment operator
     *  \param rhs another PdfError object
     *  \returns this object
     */
    PdfError& operator=(const PdfError& rhs) = default;

    /** Compares this PdfError object
     *  with an error code
     *  \param code an error code (value of the enum PdfErrorCode)
     *  \returns true if this object has the same error code.
     */
    bool operator==(PdfErrorCode code);

    /** Compares this PdfError object
     *  with an error code
     *  \param code an error code (value of the enum PdfErrorCode)
     *  \returns true if this object has a different error code.
     */
    bool operator!=(PdfErrorCode code);

    std::string_view GetName() const;

    /** Return the error code of this object.
     *  \returns the error code of this object
     */
    inline PdfErrorCode GetCode() const { return m_Code; }

    /** Get access to the internal callstack of this error.
     *  \returns the callstack deque of PdfErrorInfo objects.
     */
    inline const PdErrorInfoStack& GetCallStack() const { return m_CallStack; }

    /** Print an error message to stderr. This includes callstack
     *  and extra info, if any of either was set.
     */
    void PrintErrorMsg() const;

    /** Obtain error description.
     *  \returns a C string describing the error.
     */
    const char* what() const noexcept override;

public:
    /** Get the name for a certain error code.
     *  \returns the name or nullptr if no name for the specified
     *           error code is available.
     */
    static std::string_view ErrorName(PdfErrorCode code);

    /** Get the error message for a certain error code.
     *  \returns the error message or nullptr if no error
     *           message for the specified error code
     *           is available.
     */
    static std::string_view ErrorMessage(PdfErrorCode code);

private:
    /** Add callstack information to an error object. Always call this function
     *  if you get an error object but do not handle the error but throw it again.
     *
     *  \param filepath the filename of the source file causing
     *                 the error or nullptr. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc
     *                 macro __LINE__ here.
     *  \param information additional information on the error,
     *         e.g. how to fix the error. This string is intended to
     *         be shown to the user.
     */
    void AddToCallStack(std::string&& filepath, unsigned line, std::string&& information);

    void initFullDescription();

private:
    PdfErrorCode m_Code;
    PdErrorInfoStack m_CallStack;
    std::string m_FullDescription;
};

};

#endif // PDF_ERROR_H
