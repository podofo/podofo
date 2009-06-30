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

#include "PdfError.h"

#include <stdarg.h>
#include <stdio.h>

namespace PoDoFo {

bool PdfError::s_DgbEnabled = true;
bool PdfError::s_LogEnabled = true;

PdfErrorInfo::PdfErrorInfo()
    : m_nLine( -1 )
{
}

PdfErrorInfo::PdfErrorInfo( int line, const char* pszFile, const char* pszInfo )
    : m_nLine( line ), m_sFile( pszFile ? pszFile : "" ), m_sInfo( pszInfo ? pszInfo : "" )
{

}
 
PdfErrorInfo::PdfErrorInfo( int line, const char* pszFile, const wchar_t* pszInfo )
    : m_nLine( line ), m_sFile( pszFile ? pszFile : "" ), m_swInfo( pszInfo ? pszInfo : L"" )
{

}
PdfErrorInfo::PdfErrorInfo( const PdfErrorInfo & rhs )
{
    this->operator=( rhs );
}

const PdfErrorInfo & PdfErrorInfo::operator=( const PdfErrorInfo & rhs )
{
    m_nLine  = rhs.m_nLine;
    m_sFile  = rhs.m_sFile;
    m_sInfo  = rhs.m_sInfo;
    m_swInfo = rhs.m_swInfo;

    return *this;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------

PdfError::PdfError()
{
    m_error = ePdfError_ErrOk;
}

PdfError::PdfError( const EPdfError & eCode, const char* pszFile, int line, 
                    const char* pszInformation )
{
    this->SetError( eCode, pszFile, line, pszInformation );
}

PdfError::PdfError( const PdfError & rhs )
    : std::exception()
{
    this->operator=( rhs );
}

PdfError::~PdfError() throw()
{
}
    
const PdfError & PdfError::operator=( const PdfError & rhs )
{
    m_error     = rhs.m_error;
    m_callStack = rhs.m_callStack;

    return *this;
}

const PdfError & PdfError::operator=( const EPdfError & eCode )
{
    m_error = eCode;
    m_callStack.clear();
    
    return *this;
}

bool PdfError::operator==( const PdfError & rhs )
{
    return this->operator==( rhs.m_error );
}

bool PdfError::operator==( const EPdfError & eCode )
{
    return m_error == eCode;
}

bool PdfError::operator!=( const PdfError & rhs )
{
    return this->operator!=( rhs.m_error );
}

bool PdfError::operator!=( const EPdfError & eCode )
{
    return !this->operator==( eCode );
}

void PdfError::PrintErrorMsg() const
{
    TCIDequeErrorInfo it = m_callStack.begin();
    const char* pszMsg   = PdfError::ErrorMessage( m_error );
    const char* pszName  = PdfError::ErrorName( m_error );

    int i                = 0;

    PdfError::LogErrorMessage( eLogSeverity_Error, "\n\nPoDoFo encounter an error. Error: %i %s\n", m_error, pszName ? pszName : "" );

    if( pszMsg )
        PdfError::LogErrorMessage( eLogSeverity_Error, "\tError Description: %s\n", pszMsg );
    
    if( m_callStack.size() )
        PdfError::LogErrorMessage( eLogSeverity_Error, "\tCallstack:\n" );

    while( it != m_callStack.end() )
    {
        if( !(*it).GetFilename().empty() )
            PdfError::LogErrorMessage( eLogSeverity_Error, "\t#%i Error Source: %s:%i\n", i, (*it).GetFilename().c_str(), (*it).GetLine() );

        if( !(*it).GetInformation().empty() )
            PdfError::LogErrorMessage( eLogSeverity_Error, "\t\tInformation: %s\n", (*it).GetInformation().c_str() );

        if( !(*it).GetInformationW().empty() )
            PdfError::LogErrorMessage( eLogSeverity_Error, L"\t\tInformation: %s\n", (*it).GetInformationW().c_str() );

        ++i;
        ++it;
    }

        
    PdfError::LogErrorMessage( eLogSeverity_Error, "\n\n" );
}

const char* PdfError::what() const throw()
{
    return PdfError::ErrorName( m_error );
}

const char* PdfError::ErrorName( EPdfError eCode )
{
    const char* pszMsg = NULL;

    switch( eCode ) 
    {
        case ePdfError_ErrOk:
            pszMsg = "ePdfError_ErrOk"; 
            break;
        case ePdfError_TestFailed:
            pszMsg = "ePdfError_TestFailed"; 
            break;
        case ePdfError_InvalidHandle:
            pszMsg = "ePdfError_InvalidHandle"; 
            break;
        case ePdfError_FileNotFound:
            pszMsg = "ePdfError_FileNotFound"; 
            break;
        case ePdfError_InvalidDeviceOperation:
            pszMsg = "ePdfError_InvalidDeviceOperation";
            break;
        case ePdfError_UnexpectedEOF:
            pszMsg = "ePdfError_UnexpectedEOF"; 
            break;
        case ePdfError_OutOfMemory:
            pszMsg = "ePdfError_OutOfMemory"; 
            break;
        case ePdfError_ValueOutOfRange:
            pszMsg = "ePdfError_ValueOutOfRange"; 
            break;
        case ePdfError_InternalLogic:
            pszMsg = "ePdfError_InternalLogic";
            break;
        case ePdfError_InvalidEnumValue:
            pszMsg = "ePdfError_InvalidEnumValue";
            break;
        case ePdfError_PageNotFound:
            pszMsg = "ePdfError_PageNotFound";
            break;
        case ePdfError_NoPdfFile:
            pszMsg = "ePdfError_NoPdfFile"; 
            break;
        case ePdfError_NoXRef:
            pszMsg = "ePdfError_NoXRef"; 
            break;
        case ePdfError_NoTrailer:
            pszMsg = "ePdfError_NoTrailer"; 
            break;
        case ePdfError_NoNumber:
            pszMsg = "ePdfError_NoNumber"; 
            break;
        case ePdfError_NoObject:
            pszMsg = "ePdfError_NoObject"; 
            break;
        case ePdfError_InvalidTrailerSize:
            pszMsg = "ePdfError_InvalidTrailerSize"; 
            break;
        case ePdfError_InvalidLinearization:
            pszMsg = "ePdfError_InvalidLinearization"; 
            break;
        case ePdfError_InvalidDataType:
            pszMsg = "ePdfError_InvalidDataType"; 
            break;
        case ePdfError_InvalidXRef:
            pszMsg = "ePdfError_InvalidXRef"; 
            break;
        case ePdfError_InvalidXRefStream:
            pszMsg = "ePdfError_InvalidXRefStream"; 
            break;
        case ePdfError_InvalidXRefType:
            pszMsg = "ePdfError_InvalidXRefType"; 
            break;
        case ePdfError_InvalidPredictor:
            pszMsg = "ePdfError_InvalidPredictor"; 
            break;
        case ePdfError_InvalidStrokeStyle:
            pszMsg = "ePdfError_InvalidStrokeStyle"; 
            break;
        case ePdfError_InvalidHexString:
            pszMsg = "ePdfError_InvalidHexString"; 
            break;
        case ePdfError_InvalidStream:
            pszMsg = "ePdfError_InvalidStream"; 
            break;
        case ePdfError_InvalidStreamLength:
            pszMsg = "ePdfError_InvalidStream"; 
            break;
        case ePdfError_InvalidKey:
            pszMsg = "ePdfError_InvalidKey";
            break;
        case ePdfError_InvalidName:
            pszMsg = "ePdfError_InvalidName";
            break;
        case ePdfError_InvalidEncryptionDict:
            pszMsg = "ePdfError_InvalidEncryptionDict";    /**< The encryption dictionary is invalid or misses a required key */
            break;
        case ePdfError_InvalidPassword:                    /**< The password used to open the PDF file was invalid */
            pszMsg = "ePdfError_InvalidPassword";
            break;
        case ePdfError_InvalidFontFile:
            pszMsg = "ePdfError_InvalidFontFile";
            break;
        case ePdfError_InvalidContentStream:
            pszMsg = "ePdfError_InvalidContentStream";
            break;
        case ePdfError_UnsupportedFilter:
            pszMsg = "ePdfError_UnsupportedFilter"; 
            break;
        case ePdfError_UnsupportedFontFormat:    /**< This font format is not supported by PoDoFO. */
            pszMsg = "ePdfError_UnsupportedFontFormat";
            break;
        case ePdfError_ActionAlreadyPresent:
            pszMsg = "ePdfError_ActionAlreadyPresent"; 
            break;
        case ePdfError_MissingEndStream:
            pszMsg = "ePdfError_MissingEndStream"; 
            break;
        case ePdfError_Date:
            pszMsg = "ePdfError_Date"; 
            break;
        case ePdfError_Flate:
            pszMsg = "ePdfError_Flate"; 
            break;
        case ePdfError_FreeType:
            pszMsg = "ePdfError_FreeType"; 
            break;
        case ePdfError_SignatureError:
            pszMsg = "ePdfError_SignatureError";
            break;
        case ePdfError_MutexError:
            pszMsg = "ePdfError_MutexError";
            break;
        case ePdfError_UnsupportedImageFormat:    /**< This image format is not supported by PoDoFO. */
            pszMsg = "ePdfError_UnsupportedImageFormat";
            break;
        case ePdfError_CannotConvertColor:       /**< This color format cannot be converted. */
            pszMsg = "ePdfError_CannotConvertColor";
            break;
        case ePdfError_NotImplemented:
            pszMsg = "ePdfError_NotImplemented";
            break;
        case ePdfError_DestinationAlreadyPresent:
            pszMsg = "ePdfError_DestinationAlreadyPresent"; 
            break;
        case ePdfError_Unknown:
            pszMsg = "ePdfError_Unknown"; 
            break;
        default:
            break;
    }

    return pszMsg;
}

const char* PdfError::ErrorMessage( EPdfError eCode )
{
    const char* pszMsg = NULL;

    switch( eCode ) 
    {
        case ePdfError_ErrOk:
            pszMsg = "No error during execution.";
            break;
        case ePdfError_TestFailed:
            pszMsg = "An error curred in an automatic test included in PoDoFo.";
            break;
        case ePdfError_InvalidHandle:
            pszMsg = "A NULL handle was passed, but initialized data was expected.";
            break;
        case ePdfError_FileNotFound:
            pszMsg = "The specified file was not found.";
            break;
        case ePdfError_InvalidDeviceOperation:
            pszMsg = "Tried to do something unsupported to an I/O device like seek a non-seekable input device";
            break;
        case ePdfError_UnexpectedEOF:
            pszMsg = "End of file was reached unxexpectedly.";
            break;
        case ePdfError_OutOfMemory:
            pszMsg = "PoDoFo is out of memory.";
            break;
        case ePdfError_ValueOutOfRange:
            pszMsg = "The passed value is out of range.";
            break;
        case ePdfError_InternalLogic:
            pszMsg = "An internal error occurred.";
            break;
        case ePdfError_InvalidEnumValue:
            pszMsg = "An invalid enum value was specified.";
            break;
        case ePdfError_PageNotFound:
            pszMsg = "The requested page could not be found in the PDF.";
            break;
        case ePdfError_NoPdfFile:
            pszMsg = "This is not a PDF file.";
            break;
        case ePdfError_NoXRef:
            pszMsg = "No XRef table was found in the PDF file.";
            break;
        case ePdfError_NoTrailer:
            pszMsg = "No trailer was found in the PDF file.";
            break;
        case ePdfError_NoNumber:
            pszMsg = "A number was expected but not found.";
            break;
        case ePdfError_NoObject:
            pszMsg = "A object was expected but not found.";
            break;

        case ePdfError_InvalidTrailerSize:
        case ePdfError_InvalidLinearization:
        case ePdfError_InvalidDataType:
        case ePdfError_InvalidXRef:
        case ePdfError_InvalidXRefStream:
        case ePdfError_InvalidXRefType:
        case ePdfError_InvalidPredictor:
        case ePdfError_InvalidStrokeStyle:
        case ePdfError_InvalidHexString:
        case ePdfError_InvalidStream:
        case ePdfError_InvalidStreamLength:
        case ePdfError_InvalidKey:
        case ePdfError_InvalidName:
            break;
        case ePdfError_InvalidEncryptionDict:
            pszMsg = "The encryption dictionary is invalid or misses a required key.";
            break;
        case ePdfError_InvalidPassword:
            pszMsg = "The password used to open the PDF file was invalid.";
            break;
        case ePdfError_InvalidFontFile:
            pszMsg = "The font file is invalid.";
            break;
        case ePdfError_InvalidContentStream:
            pszMsg = "The content stream is invalid due to mismatched context pairing or other problems.";
            break;
        case ePdfError_UnsupportedFilter:
            break;
        case ePdfError_UnsupportedFontFormat:
            pszMsg = "This font format is not supported by PoDoFO.";
            break;
        case ePdfError_DestinationAlreadyPresent:
        case ePdfError_ActionAlreadyPresent:
            pszMsg = "Outlines can have either destinations or actions."; 
            break;
        case ePdfError_MissingEndStream:
        case ePdfError_Date:
            break;
        case ePdfError_Flate:
            pszMsg = "ZLib returned an error.";
            break;
        case ePdfError_FreeType:
            pszMsg = "FreeType returned an error.";
            break;
        case ePdfError_SignatureError:
            pszMsg = "The signature contains an error.";
            break;
        case ePdfError_MutexError:
            pszMsg = "Error during a mutex operation.";
            break;
        case ePdfError_UnsupportedImageFormat:
            pszMsg = "This image format is not supported by PoDoFO.";
            break;
        case ePdfError_CannotConvertColor:
            pszMsg = "This color format cannot be converted.";
            break;
        case ePdfError_NotImplemented:
            pszMsg = "This feature is currently not implemented.";
            break;
        case ePdfError_Unknown:
            pszMsg = "Error code unknown.";
        default:
            break;
    }

    return pszMsg;
}

void PdfError::LogMessage( ELogSeverity eLogSeverity, const char* pszMsg, ... )
{
	if(!PdfError::LoggingEnabled())
		return;

#ifdef DEBUG
    const ELogSeverity eMinSeverity = eLogSeverity_Debug;
#else
    const ELogSeverity eMinSeverity = eLogSeverity_Information;
#endif // DEBUG

    if( eLogSeverity < eMinSeverity )
        return;

    va_list  args;
    va_start( args, pszMsg );

    LogMessageInternal( eLogSeverity, pszMsg, args );
    va_end( args );
}

void PdfError::LogErrorMessage( ELogSeverity eLogSeverity, const char* pszMsg, ... )
{
    va_list  args;
    va_start( args, pszMsg );

    LogMessageInternal( eLogSeverity, pszMsg, args );
    va_end( args );
}

void PdfError::LogMessageInternal( ELogSeverity eLogSeverity, const char* pszMsg, va_list & args )
{
    const char* pszPrefix = NULL;

    switch( eLogSeverity ) 
    {
        case eLogSeverity_Error:
            break;
        case eLogSeverity_Critical:
	    pszPrefix = "CRITICAL: ";
            break;
        case eLogSeverity_Warning:
	    pszPrefix = "WARNING: ";
            break;
	case eLogSeverity_Information:
            break;
	case eLogSeverity_Debug:
	    pszPrefix = "DEBUG: ";
            break;
	case eLogSeverity_None:
	case eLogSeverity_Unknown:
        default:
            break;
    }

    if( pszPrefix )
        fprintf( stderr, pszPrefix );

    vfprintf( stderr, pszMsg, args );
}

void PdfError::LogMessage( ELogSeverity eLogSeverity, const wchar_t* pszMsg, ... )
{
	if(!PdfError::LoggingEnabled())
		return;

#ifdef DEBUG
    const ELogSeverity eMinSeverity = eLogSeverity_Debug;
#else
    const ELogSeverity eMinSeverity = eLogSeverity_Information;
#endif // DEBUG

    if( eLogSeverity < eMinSeverity )
        return;

    va_list  args;
    va_start( args, pszMsg );

    LogMessageInternal( eLogSeverity, pszMsg, args );
    va_end( args );
}

void PdfError::LogErrorMessage( ELogSeverity eLogSeverity, const wchar_t* pszMsg, ... )
{
    va_list  args;
    va_start( args, pszMsg );

    LogMessageInternal( eLogSeverity, pszMsg, args );
    va_end( args );
}

void PdfError::LogMessageInternal( ELogSeverity eLogSeverity, const wchar_t* pszMsg, va_list & args )
{
    const wchar_t* pszPrefix = NULL;

    switch( eLogSeverity ) 
    {
        case eLogSeverity_Error:
            break;
        case eLogSeverity_Critical:
	    pszPrefix = L"CRITICAL: ";
            break;
        case eLogSeverity_Warning:
	    pszPrefix = L"WARNING: ";
            break;
	case eLogSeverity_Information:
            break;
	case eLogSeverity_Debug:
	    pszPrefix = L"DEBUG: ";
            break;
	case eLogSeverity_None:
	case eLogSeverity_Unknown:
        default:
            break;
    }

    if( pszPrefix )
        fwprintf( stderr, pszPrefix );

    vfwprintf( stderr, pszMsg, args );
}

void PdfError::DebugMessage( const char* pszMsg, ... )
{
	if ( !PdfError::DebugEnabled() )		
            return;

	const char* pszPrefix = "DEBUG: ";

	va_list  args;
	va_start( args, pszMsg );

	if( pszPrefix )
		fprintf( stderr, pszPrefix );

	vfprintf( stderr, pszMsg, args );
	va_end( args );
}

};
