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
 ***************************************************************************/

#ifndef _PDF_ERROR_H_
#define _PDF_ERROR_H_

#include "PdfDefines.h"
#include <queue>

/** \file PdfError.h
 *  Error information and logging is implemented in this file.
 */

namespace PoDoFo {

/** Error Code defines which are used in PdfError to describe the error.
 *
 *  If you add an error code to this enum, please also add it to PdfError::ErrorName
 *  and PdfError::ErrorMessage.
 * 
 *  \see PdfError
 */
typedef enum EPdfError {
    ePdfError_ErrOk = 0,                /**< The default value indicating no error. */

    ePdfError_TestFailed,               /**< Used in PoDoFo tests, to indicate that a test failed for some reason. */

    ePdfError_InvalidHandle,            /**< Null pointer was passed, but null pointer is not allowed. */
    ePdfError_FileNotFound,             /**< A file was not found or cannot be opened. */
    ePdfError_UnexpectedEOF,            /**< End of file was reached but data was expected. */
    ePdfError_OutOfMemory,              /**< Not enough memory to complete an operation. */
    ePdfError_ValueOutOfRange,          /**< The specified memory is out of the allowed range. */

    ePdfError_NoPdfFile,                /**< The file is no PDF file. */
    ePdfError_NoXRef,                   /**< The PDF file has no or an invalid XRef table. */
    ePdfError_NoTrailer,                /**< The PDF file has no or an invalid trailer. */
    ePdfError_NoNumber,                 /**< A number was expected in the PDF file, but the read string is no number. */
    ePdfError_NoObject,                 /**< A object was expected and non was found. */

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
    ePdfError_InvalidStreamLength,      /**< The stream length is invlaid */
    ePdfError_InvalidKey,               /**< The specified key is invalid */

    ePdfError_UnsupportedFilter,        /**< The requested filter is not yet implemented. */

    ePdfError_MissingEndStream,         /**< The required token endstream was not found. */
    ePdfError_Date,                     /**< Date/time error */
    ePdfError_Flate,                    /**< Error in zlib */
    ePdfError_FreeType,                 /**< Error in FreeType */
    ePdfError_SignatureError,           /**< Error in signature */
  
    ePdfError_Unknown = 0xffff          /**< Unknown error */
};

/**
 * Used in PdfError::LogMessage to specify the log level.
 *
 * \see PdfError::LogMessage
 */
typedef enum ELogSeverity {
    eLogSeverity_Critical,            /**< Critical unexpected error */
    eLogSeverity_Error,               /**< Error */
    eLogSeverity_Warning,             /**< Warning */
    eLogSeverity_Information,         /**< Information message */
    eLogSeverity_Debug,               /**< Debug information */
    eLogSeverity_None,                /**< No specified level */

    eLogSeverity_Unknown = 0xffff     /**< Unknown log level */
};

/** \def RAISE_ERROR( x )
 *  
 *  Set the value of the variable eCode (which has to exist in the current function) to x
 *  and return the eCode.
 */
#define RAISE_ERROR( x ) eCode.SetError( x, __FILE__, __LINE__ ); return eCode;

/** \def RAISE_ERROR_INFO( x, y )
 *  
 *  Set the value of the variable eCode (which has to exist in the current function) to x
 *  and return the eCode. Additionally additional information on the error y is set. y has 
 *  to be an c-string.
 */
#define RAISE_ERROR_INFO( x, y ) eCode.SetError( x, __FILE__, __LINE__, y ); return eCode;

class PdfErrorInfo {
 public:
    PdfErrorInfo();
    PdfErrorInfo( int line, const char* pszFile, const char* pszInfo );
    PdfErrorInfo( const PdfErrorInfo & rhs );

    const PdfErrorInfo & operator=( const PdfErrorInfo & rhs );

    inline int Line() const { return m_nLine; }
    inline const std::string & Filename() const { return m_sFile; }
    inline const std::string & Information() const { return m_sInfo; }

    inline void SetInformation( const char* pszInfo ) { m_sInfo = pszInfo ? pszInfo : ""; }

 private:
    int         m_nLine;
    std::string m_sFile;
    std::string m_sInfo;
};


typedef std::deque<PdfErrorInfo>        TDequeErrorInfo;
typedef TDequeErrorInfo::iterator       TIDequeErrorInfo;
typedef TDequeErrorInfo::const_iterator TCIDequeErrorInfo;

/** The error handling class of PoDoFo lib.
 *  Whenever a function encounters an error
 *  a PdfError object is returned.
 *  
 *  A PdfError with Error() == ErrOk means
 *  successfull execution.
 *
 *  This class provides also meaningfull
 *  error descriptions.
 */
class PdfError {
 public:
    /** Create a PdfError object initialized to ErrOk
     */
    PdfError();

    /** Create a PdfError object with a given error code.
     *  \param eCode the error code of this object
     */
    PdfError( const EPdfError & eCode );

    /** Copy constructor
     *  \param rhs copy the contents of rhs into this object
     */
    PdfError( const PdfError & rhs );

    virtual ~PdfError();
    
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

    /** Comparison operator compares 2 PdfError objects
     *  \param rhs another PdfError object
     *  \returns true if both objects have the same error code.
     */
    bool operator==( const PdfError & rhs );

    /** Overloaded comparison operator compares 2 PdfError objects
     *  \param eCode an erroce code
     *  \returns true if this object has the same error code.
     */
    bool operator==( const EPdfError & eCode );

    /** Comparison operator compares 2 PdfError objects
     *  \param rhs another PdfError object
     *  \returns true if both objects have the different error code.
     */
    bool operator!=( const PdfError & rhs );

    /** Overloaded comparison operator compares 2 PdfError objects
     *  \param eCode an erroce code
     *  \returns true if this object has different error code.
     */
    bool operator!=( const EPdfError & eCode );

    /** Return the error code of this object
     *  \returns the error code of this object
     */
    inline EPdfError Error() const;

    inline const TDequeErrorInfo & Callstack() const;

    /** Set the error code of this object.
     *  \param eCode the error code of this object
     *  \param pszFile the filename of the source file causing
     *                 the error or NULL. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc 
     *                 macro __LINE__ here.
     *  \param pszInformation additional information on the error.
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void SetError( const EPdfError & eCode, const char* pszFile = NULL, int line = 0, const char* pszInformation = NULL );

    /** Set additional error informatiom
     *  \param pszInformation additional information on the error.
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void SetErrorInformation( const char* pszInformation );

    /** Add callstack information to an error object. Always call this function
     *  if you get an error object but do not handle the error but throw it again.
     *
     *  \param pszFile the filename of the source file causing
     *                 the error or NULL. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc 
     *                 macro __LINE__ here.
     *  \param pszInformation additional information on the error.
     *         e.g. how to fix the error. This string is intended to 
     *         be shown to the user.
     */
    inline void AddToCallstack( const char* pszFile = NULL, int line = 0, const char* pszInformation = NULL );

    /** \returns true if an error code was set 
     *           and false if the error code is ePdfError_ErrOk
     */
    inline bool IsError() const;

    /** Print an error message to stderr
     */
    void PrintErrorMsg() const;
    
    /** Get the name for a certain error code.
     *  \returns the name or NULL if no name for the specified
     *           error code is available.
     */
    static const char* ErrorName( EPdfError eCode );

    /** Get the error message for a certain error code.
     *  \returns the error message or NULL if no error
     *           message for the specified error code
     *           is available.
     */
    static const char* ErrorMessage( EPdfError eCode );

    /** Log a message to the logging system defined for PoDoFo.
     *  \param eLogSeverity the sevirity of the log message
     *  \param pszMsg       the message to be logged
     */
    static void LogMessage( ELogSeverity eLogSeverity, const char* pszMsg, ... );

    /** Log a message to the logging system defined for PoDoFo for debugging
     *  \param pszMsg       the message to be logged
     */
    static void DebugMessage( const char* pszMsg, ... );

    /** Enable or disable the display of debugging messages
     *  \param bEnable       enable (true) or disable (false)
     */
    static void EnableDebug( bool bEnable ) { PdfError::s_DgbEnabled = bEnable; }
	
    /** Is the display of debugging messages enabled or not?
     */
    static bool DebugEnabled() { return PdfError::s_DgbEnabled; }

 private:
    EPdfError          m_error;

    TDequeErrorInfo    m_callStack;

    static bool        s_DgbEnabled;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfError PdfError::Error() const
{
    return m_error;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const TDequeErrorInfo & PdfError::Callstack() const
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
void PdfError::SetErrorInformation( const char* pszInformation )
{
    if( m_callStack.size() )
        m_callStack.front().SetInformation( pszInformation ? pszInformation : "" );
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

