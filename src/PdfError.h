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

namespace PoDoFo {

typedef enum EPdfError {
    ePdfError_ErrOk = 0,

    ePdfError_TestFailed, 

    ePdfError_InvalidHandle,
    ePdfError_FileNotFound,
    ePdfError_UnexpectedEOF,
    ePdfError_OutOfMemory,
    ePdfError_ValueOutOfRange,

    ePdfError_NoPdfFile,
    ePdfError_NoXRef,
    ePdfError_NoTrailer,
    ePdfError_NoNumber,
    ePdfError_NoObject,

    ePdfError_InvalidTrailerSize,
    ePdfError_InvalidLinearization,
    ePdfError_InvalidDataType,
    ePdfError_InvalidXRef,
    ePdfError_InvalidXRefStream,
    ePdfError_InvalidXRefType,
    ePdfError_InvalidPredictor,
    ePdfError_InvalidStrokeStyle,
    ePdfError_InvalidHexString,
    ePdfError_InvalidStream,
    ePdfError_InvalidStreamLength,
    ePdfError_InvalidKey,

    ePdfError_UnsupportedFilter,

    ePdfError_MissingEndStream,
    ePdfError_Date,
    ePdfError_Flate,
    ePdfError_FreeType,
    ePdfError_SignatureError,

    ePdfError_Unknown = 0xffff
};

typedef enum ELogSeverity {
    eLogSeverity_Critical,
    eLogSeverity_Error,
    eLogSeverity_Warning,
    eLogSeverity_Information,
    eLogSeverity_Debug,
    eLogSeverity_None,

    eLogSeverity_Unknown = 0xffff
};

#define RAISE_ERROR( x ) eCode.SetError( x, __FILE__, __LINE__ ); return eCode;
#define RAISE_ERROR_INFO( x, y ) eCode.SetError( x, __FILE__, __LINE__, y ); return eCode;

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

    /** \returns true if an error code was set 
     *           and false if the error code is ePdfError_ErrOk
     */
    inline bool IsError() const;

    /** Print an error message to stderr
     */
    void PrintErrorMsg() const;

    /** Get the error message for a certain error code.
     *  \returns the error message or NULL if no error
     *           message for the specified error code
     *           is available.
     */
    static const char* ErrorMessage( EPdfError eCode );

    /** Get the name for a certain error code.
     *  \returns the name or NULL if no name for the specified 
     *           error code is available.
     */
    static const char* ErrorName( EPdfError eCode );

    /** \returns the source filename where the error occurred
     *           or NULL if none was set.
     */
    static const char* Filename();

    /** \returns the source line where the error occurred
     *           or 0 if none was set.
     */
    static const int Line();

    /** \returns additional information on the error
     *           e.g. how to fix it.
     */
    static const char* Information();

    /** Log a message to the logging system defined for PoDoFo.
     *  \param eLogSeverity the sevirity of the log message
     *  \param pszMsg       the message to be logged
     */
    static void LogMessage( ELogSeverity eLogSeverity, const char* pszMsg, ... );

 private:
    EPdfError m_error;

    static int         s_line;
    static const char* s_file;
    static std::string s_info;
};

EPdfError PdfError::Error() const
{
    return m_error;
}

void PdfError::SetError( const EPdfError & eCode, const char* pszFile, int line, const char* pszInformation )
{
    m_error = eCode;
    s_file  = pszFile;
    s_line  = line;
    s_info  = pszInformation ? pszInformation : "";
}

void PdfError::SetErrorInformation( const char* pszInformation )
{
    s_info  = pszInformation;
}

bool PdfError::IsError() const
{
    return (m_error != ePdfError_ErrOk);
}

};

#endif /* _PDF_ERROR_H_ */

