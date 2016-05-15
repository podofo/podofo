/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                      by Petr Pytelka                                    *
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

#ifndef _PODOFO_SIGN_OUTPUT_DEVICE_H_
#define _PODOFO_SIGN_OUTPUT_DEVICE_H_

#include "../base/PdfOutputDevice.h"
#include "../base/PdfData.h"
#include "../base/PdfString.h"

namespace PoDoFo 
{

/** Signer class
 *
 * Class is used to locate place for signature in the stream.
 * Usage:
 * 1. Locate signature and adjust ByteRange
 * 2. Generate signature
 * 3. Insert new signature
 */
class PODOFO_DOC_API PdfSignOutputDevice :public PdfOutputDevice 
{
public:
    PdfSignOutputDevice(PdfOutputDevice *pRealDevice);
    PdfSignOutputDevice(const char* pszFilename);
#ifdef WIN32
    PdfSignOutputDevice( const wchar_t* pszFilename );
#endif
    virtual ~PdfSignOutputDevice();

    /** Set string to lookup for 
     * 
     * \param lSignatureSize Total space reserved for signature
     */
    virtual void SetSignatureSize(size_t lSignatureSize);

    /** Get expected size of signature.
     * 
	 * If reserved size if zero no signatures will be added.
     * \return Total space reserved for signature.
     */
	virtual size_t GetSignatureSize()const;

    /** Return signature beacon
     * 
     */
    virtual const PdfData *GetSignatureBeacon()const{ return m_pSignatureBeacon; }

    virtual bool HasSignaturePosition()const { return m_bBeaconFound; }

    /** Modify ByteRange entry according to signature position
     *
     */
    virtual void AdjustByteRange();

    /** Read data for signature
     */
    virtual size_t ReadForSignature(char* pBuffer, size_t lLen);

    /** Embed real signature in the PDF
     */
    virtual void SetSignature(const PdfData &sigData);

    virtual inline size_t GetLength() const
    {
        return m_pRealDevice->GetLength();
    }
    virtual void Print( const char* pszFormat, ... )
    {
        va_list args;
        long lBytes;
        
        va_start( args, pszFormat );
        lBytes = m_pRealDevice->PrintVLen(pszFormat, args);
        va_end( args );
        
        va_start( args, pszFormat );
        m_pRealDevice->PrintV(pszFormat, lBytes, args);
        va_end( args );
    }
    
    virtual void Write( const char* pBuffer, size_t lLen );
    virtual size_t Read( char* pBuffer, size_t lLen )
    {
        return m_pRealDevice->Read(pBuffer, lLen);
    }
    virtual void Seek( size_t offset )
    {
        m_pRealDevice->Seek(offset);
    }

    virtual inline size_t Tell() const
    {
        return m_pRealDevice->Tell();
    }
    virtual void Flush()
    {
        m_pRealDevice->Flush();
    }

private:
    void Init();

    PdfOutputDevice* m_pRealDevice;
    bool m_bDevOwner;
    PdfData* m_pSignatureBeacon;
    size_t m_sBeaconPos;
    bool m_bBeaconFound;
};

}

#endif
