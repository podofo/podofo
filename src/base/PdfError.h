/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_ERROR_H_
#define _PDF_ERROR_H_

// PdfError.h should not include PdfDefines.h, since it is included by it.
// It should avoid depending on anything defined in PdfDefines.h .

#include "podofoapi.h"
#include <string>
#include <queue>
#include <cstdarg>

#if defined(_MSC_VER)  &&  _MSC_VER <= 1200 // same pragma as in PdfDefines.h which we cannot include here
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#endif

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
enum EPdfError {
    ePdfError_ErrOk = 0,                /**< The default value indicating no error. */

    ePdfError_TestFailed,               /**< Used in PoDoFo tests, to indicate that a test failed for some reason. */

    ePdfError_InvalidHandle,            /**< Null pointer was passed, but null pointer is not allowed. */
    ePdfError_FileNotFound,             /**< A file was not found or cannot be opened. */
    ePdfError_InvalidDeviceOperation,	/**< Tried to do something unsupported to an I/O device like seek a non-seekable input device */
    ePdfError_UnexpectedEOF,            /**< End of file was reached but data was expected. */
    ePdfError_OutOfMemory,              /**< Not enough memory to complete an operation. */
    ePdfError_ValueOutOfRange,          /**< The specified memory is out of the allowed range. */
    ePdfError_InternalLogic,            /**< An internal sanity check or assertion failed. */ 
    ePdfError_InvalidEnumValue,         /**< An invalid enum value was specified. */
    ePdfError_BrokenFile,               /**< The file content is broken. */

    ePdfError_PageNotFound,             /**< The requested page could not be found in the PDF. */

    ePdfError_NoPdfFile,                /**< The file is no PDF file. */
    ePdfError_NoXRef,                   /**< The PDF file has no or an invalid XRef table. */
    ePdfError_NoTrailer,                /**< The PDF file has no or an invalid trailer. */
    ePdfError_NoNumber,                 /**< A number was expected in the PDF file, but the read string is no number. */
    ePdfError_NoObject,                 /**< A object was expected and none was found. */
    ePdfError_NoEOFToken,               /**< The PDF file has no or an invalid EOF marker. */

    ePdfError_InvalidTrailerSize,       /**< The trailer size is invalid. */
    ePdfError_InvalidLinearization,     /**< The linearization directory of a web-optimized PDF file is invalid. */
    ePdfError_InvalidDataType,          /**< The passed datatype is invalid or was not recognized */
    ePdfError_InvalidXRef,              /**< The XRef table is invalid */
    ePdfError_InvalidXRefStream,        /**< A XRef steam is invalid */
    ePdfError_InvalidXRefType,          /**< The XRef type is invalid or was not found */
    ePdfError_InvalidPredictor,         /**< Invalid or unimplemented predictor */
    ePdfError_InvalidStrokeStyle,       /**< Invalid stroke style during drawing */
    ePdfError_InvalidHexString,         /**< Invalid hex string */
    ePdfError_InvalidStream,            /**< The stream is invalid */
    ePdfError_InvalidStreamLength,      /**< The stream length is invalid */
    ePdfError_InvalidKey,               /**< The specified key is invalid */
    ePdfError_InvalidName,              /**< The specified Name is not valid in this context */
    ePdfError_InvalidEncryptionDict,    /**< The encryption dictionary is invalid or misses a required key */
    ePdfError_InvalidPassword,          /**< The password used to open the PDF file was invalid */
    ePdfError_InvalidFontFile,          /**< The font file is invalid */
    ePdfError_InvalidContentStream,     /**< The content stream is invalid due to mismatched context pairing or other problems */

    ePdfError_UnsupportedFilter,        /**< The requested filter is not yet implemented. */
    ePdfError_UnsupportedFontFormat,    /**< This font format is not supported by PoDoFo. */
    ePdfError_ActionAlreadyPresent,     /**< An Action was already present when trying to add a Destination */
    ePdfError_WrongDestinationType,     /**< The requested field is not available for the given destination type */

    ePdfError_MissingEndStream,         /**< The required token endstream was not found. */
    ePdfError_Date,                     /**< Date/time error */
    ePdfError_Flate,                    /**< Error in zlib */
    ePdfError_FreeType,                 /**< Error in FreeType */
    ePdfError_SignatureError,           /**< Error in signature */

    ePdfError_MutexError,               /**< Error during a mutex operation */

    ePdfError_UnsupportedImageFormat,   /**< This image format is not supported by PoDoFo. */
    ePdfError_CannotConvertColor,       /**< This color format cannot be converted. */

    ePdfError_NotImplemented,           /**< This feature is currently not implemented. */

    ePdfError_DestinationAlreadyPresent,/**< A destination was already present when trying to add an Action */
    ePdfError_ChangeOnImmutable,        /**< Changing values on immutable objects is not allowed. */

    ePdfError_NotCompiled,              /**< This feature was disabled at compile time. */

    ePdfError_OutlineItemAlreadyPresent,/**< An outline item to be inserted was already in that outlines tree. */
    ePdfError_NotLoadedForUpdate,       /**< The document had not been loaded for update. */
    ePdfError_CannotEncryptedForUpdate, /**< Cannot load encrypted documents for update. */

    ePdfError_Unknown = 0xffff          /**< Unknown error */
};

/**
 * Used in PdfError::LogMessage to specify the log level.
 *
 * \see PdfError::LogMessage
 */
enum ELogSeverity {
    eLogSeverity_Critical,            /**< Critical unexpected error */
    eLogSeverity_Error,               /**< Error */
    eLogSeverity_Warning,             /**< Warning */
    eLogSeverity_Information,         /**< Information message */
    eLogSeverity_Debug,               /**< Debug information */
    eLogSeverity_None,                /**< No specified level */

    eLogSeverity_Unknown = 0xffff     /**< Unknown log level */
};

/** \def PODOFO_RAISE_ERROR( x )
 *  
 *  Throw an exception of type PdfError with the error code x, which should be
 *  one of the values of the enum EPdfError. File and line info are included.
 */
#define PODOFO_RAISE_ERROR( x ) throw ::PoDoFo::PdfError( x, __FILE__, __LINE__ );

/** \def PODOFO_RAISE_ERROR_INFO( x, y )
 *  
 *  Throw an exception of type PdfError with the error code x, which should be
 *  one of the values of the enum EPdfError. File and line info are included.
 *  Additionally extra information on the error, y is set, which will also be
 *  output by PdfError::PrintErrorMsg().
 *  y can be a C string, but can also be a C++ std::string.
 */
#define PODOFO_RAISE_ERROR_INFO( x, y ) throw ::PoDoFo::PdfError( x, __FILE__, __LINE__, y );

/** \def PODOFO_RAISE_LOGIC_IF( x, y )
 *
 *  Evaluate `x' as a binary predicate and if it is true, raise a logic error with the
 *  info string `y' .
 */
#define PODOFO_RAISE_LOGIC_IF( x, y ) { if (x) throw ::PoDoFo::PdfError( ePdfError_InternalLogic, __FILE__, __LINE__, y ); };

class PODOFO_API PdfErrorInfo {
 public:
    PdfErrorInfo();
    PdfErrorInfo( int line, const char* pszFile, const char* pszInfo );
    PdfErrorInfo( int line, const char* pszFile, std::string pszInfo );
    PdfErrorInfo( int line, const char* pszFile, const wchar_t* pszInfo );
    PdfErrorInfo( const PdfErrorInfo & rhs );

    const PdfErrorInfo & operator=( const PdfErrorInfo & rhs );

    inline int GetLine() const { return m_nLine; }
    inline const std::string & GetFilename() const { return m_sFile; }
    inline const std::string & GetInformation() const { return m_sInfo; }
    inline const std::wstring & GetInformationW() const { return m_swInfo; }

    inline void SetInformation( const char* pszInfo ) { m_sInfo = pszInfo ? pszInfo : ""; }
    inline void SetInformation( std::string pszInfo ) { m_sInfo = pszInfo; }
    inline void SetInformation( const wchar_t* pszInfo ) { m_swInfo = pszInfo ? pszInfo : L""; }

 private:
    int          m_nLine;
    std::string  m_sFile;
    std::string  m_sInfo;
    std::wstring m_swInfo;
};


typedef std::deque<PdfErrorInfo>        TDequeErrorInfo;
typedef TDequeErrorInfo::iterator       TIDequeErrorInfo;
typedef TDequeErrorInfo::const_iterator TCIDequeErrorInfo;


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
 *  for the error codes which are values of the enum EPdfError,
 *  which are all codes PoDoFo uses (except the first and last one).
 */
class PODOFO_EXCEPTION_API_DOXYGEN PdfError {
 public:

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    class LogMessageCallback
    {
    public:
        virtual ~LogMessageCallback() {} // every class with virtual methods needs a virtual destructor
        virtual void LogMessage( ELogSeverity eLogSeverity, const char* pszPrefix, const char* pszMsg, va_list & args ) = 0;
        virtual void LogMessage( ELogSeverity eLogSeverity, const wchar_t* pszPrefix, const wchar_t* pszMsg, va_list & args ) = 0;
    };

    /** Set a global static LogMessageCallback functor to replace stderr output in LogMessageInternal.
     *  \param fLogMessageCallback the pointer to the new callback functor object
     *  \returns the pointer to the previous callback functor object
     */
    static LogMessageCallback* SetLogMessageCallback(LogMessageCallback* fLogMessageCallback);

    /** Create a PdfError object initialized to ePdfError_ErrOk.
     */
    PdfError();

    /** Create a PdfError object with a given error code.
     *  \param eCode the error code of this object
     *  \param pszFile the file in which the error has occured. 
     *         Use the compiler macro __FILE__ to initialize the field.
     *  \param line the line in which the error has occured.
     *         Use the compiler macro __LINE__ to initialize the field.
     *  \param pszInformation additional information on this error
     */
    PdfError( const EPdfError & eCode, const char* pszFile = NULL, int line = 0, 
              const char* pszInformation = NULL );

    /** Create a PdfError object with a given error code.
     *  \param eCode the error code of this object
     *  \param pszFile the file in which the error has occured. 
     *         Use the compiler macro __FILE__ to initialize the field.
     *  \param line the line in which the error has occured.
     *         Use the compiler macro __LINE__ to initialize the field.
     *  \param sInformation additional information on this error
     */
    explicit PdfError( const EPdfError & eCode, const char* pszFile, int line, 
                        std::string sInformation );

    /** Copy constructor
     *  \param rhs copy the contents of rhs into this object
     */
    PdfError( const PdfError & rhs );

    virtual ~PdfError() throw();
    
    /** Assignment operator
     *  \param rhs another PdfError object
     *  \returns this object
     */
    const PdfError & operator=( const PdfError & rhs );

    /** Overloaded assignment operator
     *  \param eCode a EPdfError code
     *  \returns this object
     */
    const PdfError & operator=( const EPdfError & eCode );

    /** Comparison operator, compares 2 PdfError objects
     *  \param rhs another PdfError object
     *  \returns true if both objects have the same error code.
     */
    bool operator==( const PdfError & rhs );

    /** Overloaded comparison operator, compares this PdfError object
     *  with an error code
     *  \param eCode an error code (value of the enum EPdfError)
     *  \returns true if this object has the same error code.
     */
    bool operator==( const EPdfError & eCode );

    /** Comparison operator, compares 2 PdfError objects
     *  \param rhs another PdfError object
     *  \returns true if the objects have different error codes.
     */
    bool operator!=( const PdfError & rhs );

    /** Overloaded comparison operator, compares this PdfError object
     *  with an error code
     *  \param eCode an error code (value of the enum EPdfError)
     *  \returns true if this object has a different error code.
     */
    bool operator!=( const EPdfError & eCode );

    /** Return the error code of this object.
     *  \returns the error code of this object
     */
    inline EPdfError GetError() const;

    /** Get access to the internal callstack of this error.
     *  \returns the callstack deque of PdfErrorInfo objects.
     */
    inline const TDequeErrorInfo & GetCallstack() const;

    /** Set the error code of this object.
     *  \param eCode the error code of this object
     *  \param pszFile the filename of the source file causing
     *                 the error or NULL. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc 
     *                 macro __LINE__ here.
     *  \param sInformation additional information on the error.
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void SetError( const EPdfError & eCode, const char* pszFile, int line,
                        std::string sInformation );

    /** Set the error code of this object.
     *  \param eCode the error code of this object
     *  \param pszFile the filename of the source file causing
     *                 the error or NULL. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc 
     *                 macro __LINE__ here.
     *  \param pszInformation additional information on the error,
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void SetError( const EPdfError & eCode, const char* pszFile = NULL, int line = 0, const char* pszInformation = NULL );

    /** Set additional error information.
     *  \param pszInformation additional information on the error,
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void SetErrorInformation( const char* pszInformation );

    /** Set additional error information.
     *  \param pszInformation additional information on the error,
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void SetErrorInformation( const wchar_t* pszInformation );

	/** Add callstack information to an error object. Always call this function
     *  if you get an error object but do not handle the error but throw it again.
     *
     *  \param pszFile the filename of the source file causing
     *                 the error or NULL. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc 
     *                 macro __LINE__ here.
     *  \param pszInformation additional information on the error,
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void AddToCallstack( const char* pszFile = NULL, int line = 0, const char* pszInformation = NULL );

	/** Add callstack information to an error object. Always call this function
     *  if you get an error object but do not handle the error but throw it again.
     *
     *  \param pszFile the filename of the source file causing
     *                 the error or NULL. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc 
     *                 macro __LINE__ here.
     *  \param sInformation additional information on the error,
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void AddToCallstack( const char* pszFile, int line, std::string sInformation );

    /** \returns true if an error code was set 
     *           and false if the error code is ePdfError_ErrOk.
     */
    inline bool IsError() const;

    /** Print an error message to stderr. This includes callstack
     *  and extra info, if any of either was set.
     */
    void PrintErrorMsg() const;

    /** Obtain error description.
     *  \returns a C string describing the error.
     */
    const char* what() const;

    /** Get the name for a certain error code.
     *  \returns the name or NULL if no name for the specified
     *           error code is available.
     */
    PODOFO_NOTHROW static const char* ErrorName( EPdfError eCode );

    /** Get the error message for a certain error code.
     *  \returns the error message or NULL if no error
     *           message for the specified error code
     *           is available.
     */
    static const char* ErrorMessage( EPdfError eCode );

    /** Log a message to the logging system defined for PoDoFo.
     *  \param eLogSeverity the severity of the log message
     *  \param pszMsg       the message to be logged
     */
    static void LogMessage( ELogSeverity eLogSeverity, const char* pszMsg, ... );

    /** Log a message to the logging system defined for PoDoFo.
     *  \param eLogSeverity the severity of the log message
     *  \param pszMsg       the message to be logged
     */
    static void LogMessage( ELogSeverity eLogSeverity, const wchar_t* pszMsg, ... );

     /** Enable or disable logging.
     *  \param bEnable       enable (true) or disable (false)
     */
    static void EnableLogging( bool bEnable );
	
    /** Is the display of debugging messages enabled or not?
     */
    static bool LoggingEnabled();
    
    /** Log a message to the logging system defined for PoDoFo for debugging.
     *  \param pszMsg       the message to be logged
     */
    static void DebugMessage( const char* pszMsg, ... );

    /** Enable or disable the display of debugging messages.
     *  \param bEnable       enable (true) or disable (false)
     */
    static void EnableDebug( bool bEnable );
	
    /** Is the display of debugging messages enabled or not?
     */
    static bool DebugEnabled();

 private:
    /** Log a message to the logging system defined for PoDoFo.
     *
     *  This call does not check if logging is enabled and always
     *  prints the error message.
     *
     *  \param eLogSeverity the severity of the log message
     *  \param pszMsg       the message to be logged
     */
    static void LogErrorMessage( ELogSeverity eLogSeverity, const char* pszMsg, ... );

    /** Log a message to the logging system defined for PoDoFo.
     *
     *  This call does not check if logging is enabled and always
     *  prints the error message
     *
     *  \param eLogSeverity the severity of the log message
     *  \param pszMsg       the message to be logged
     */
    static void LogErrorMessage( ELogSeverity eLogSeverity, const wchar_t* pszMsg, ... );

    static void LogMessageInternal( ELogSeverity eLogSeverity, const char* pszMsg, va_list & args );
    static void LogMessageInternal( ELogSeverity eLogSeverity, const wchar_t* pszMsg, va_list & args );

 private:
    EPdfError          m_error;

    TDequeErrorInfo    m_callStack;

    static bool        s_DgbEnabled;
    static bool        s_LogEnabled;

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    static LogMessageCallback* m_fLogMessageCallback;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfError PdfError::GetError() const
{
    return m_error;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const TDequeErrorInfo & PdfError::GetCallstack() const
{
    return m_callStack;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfError::SetError( const EPdfError & eCode, const char* pszFile, int line, const char* pszInformation )
{
    m_error = eCode;
    this->AddToCallstack( pszFile, line, pszInformation );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfError::AddToCallstack( const char* pszFile, int line, const char* pszInformation )
{
    m_callStack.push_front( PdfErrorInfo( line, pszFile, pszInformation ) );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfError::SetError( const EPdfError & eCode, const char* pszFile, int line, std::string sInformation )
{
    m_error = eCode;
    this->AddToCallstack( pszFile, line, sInformation );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfError::AddToCallstack( const char* pszFile, int line, std::string sInformation )
{
    m_callStack.push_front( PdfErrorInfo( line, pszFile, sInformation ) );
}
// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfError::SetErrorInformation( const char* pszInformation )
{
    if( m_callStack.size() )
        m_callStack.front().SetInformation( pszInformation ? pszInformation : "" );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfError::SetErrorInformation( const wchar_t* pszInformation )
{
    if( m_callStack.size() )
        m_callStack.front().SetInformation( pszInformation ? pszInformation : L"" );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfError::IsError() const
{
    return (m_error != ePdfError_ErrOk);
}

};

#endif /* _PDF_ERROR_H_ */



