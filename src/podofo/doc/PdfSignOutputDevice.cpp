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

#include "PdfSignOutputDevice.h"
#include "../base/PdfArray.h"

#include <string.h>

namespace PoDoFo {


PdfSignOutputDevice::PdfSignOutputDevice(PdfOutputDevice *pRealDevice)
{
    Init();
    m_pRealDevice = pRealDevice;	
}

PdfSignOutputDevice::PdfSignOutputDevice(const char* pszFilename)
{
    Init();
    m_pRealDevice = new PdfOutputDevice(pszFilename);
    m_bDevOwner = true;
}

#ifdef WIN32
PdfSignOutputDevice::PdfSignOutputDevice( const wchar_t* pszFilename )
{
    Init();
    m_pRealDevice = new PdfOutputDevice(pszFilename);
    m_bDevOwner = true;
}
#endif

void PdfSignOutputDevice::Init()
{
    m_pSignatureBeacon = NULL;
    m_bBeaconFound = false;
    m_bDevOwner = false;
    m_sBeaconPos = 0;
}

PdfSignOutputDevice::~PdfSignOutputDevice()
{
    if(m_pSignatureBeacon!=NULL) {
        delete m_pSignatureBeacon;
    }
    if(m_bDevOwner)
    {
        delete m_pRealDevice;
    }
}

void PdfSignOutputDevice::SetSignatureSize(size_t lSignatureSize)
{	
    if(m_pSignatureBeacon!=NULL) {
        delete m_pSignatureBeacon;
    }
    const char srcBeacon[] = "###HERE_WILL_BE_SIGNATURE___";	
    size_t lLen = sizeof(srcBeacon);

	lSignatureSize = 2*lSignatureSize;
    char* pData = static_cast<char*>(podofo_malloc(lSignatureSize));
 	if (!pData)
 	{
 		PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
    }
    
    for(size_t i=0; i<lSignatureSize; i++)
    {
        pData[i]=srcBeacon[i%lLen];
    }
    m_pSignatureBeacon = new PdfData(pData, lSignatureSize);
    podofo_free(pData);
}

size_t PdfSignOutputDevice::GetSignatureSize()const
{
	return (m_pSignatureBeacon == NULL) ? 0 : ( m_pSignatureBeacon->data().size() / 2 );
}

void PdfSignOutputDevice::SetSignature(const PdfData &sigData)
{
    if(!m_bBeaconFound) {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }
    size_t maxSigSize = m_pSignatureBeacon->data().size();
    size_t sigByteSize = sigData.data().size();
    // check signature size
    if((sigByteSize*2)> maxSigSize) {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }
    PdfString sig(sigData.data().c_str(), sigByteSize, true);

    m_pRealDevice->Seek(m_sBeaconPos);
    sig.Write(m_pRealDevice, PoDoFo::ePdfWriteMode_Compact);
    // insert padding
    size_t numPadding = maxSigSize-2*sigByteSize;
    if(numPadding>0) {
        // Seek back
        m_pRealDevice->Seek(m_pRealDevice->Tell()-1);
        while(numPadding>0) {
            char c='0';
            m_pRealDevice->Write(&c, 1);
            numPadding--;
        }
    }
}

void PdfSignOutputDevice::AdjustByteRange()
{
    if(!m_bBeaconFound) {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }

    // Get final position
    size_t sFileEnd = GetLength();
    PdfArray arr;
    arr.push_back( PdfVariant(static_cast<pdf_int64>(0)) );
    arr.push_back( PdfVariant(static_cast<pdf_int64>(m_sBeaconPos)) );
    arr.push_back( PdfVariant(static_cast<pdf_int64>(m_sBeaconPos+m_pSignatureBeacon->data().size()+2) ) );
    arr.push_back( PdfVariant(static_cast<pdf_int64>(sFileEnd-(m_sBeaconPos+m_pSignatureBeacon->data().size()+2)) ) );
    std::string sPosition;
    PdfVariant(arr).ToString(sPosition, ePdfWriteMode_Compact);
    // Fill padding
    unsigned int sPosSize = sizeof("[ 0 1234567890 1234567890 1234567890]")-1;
    if(sPosition.size()<sPosSize)
    {
        // drop last ']'
        sPosition.resize(sPosition.size()-1);
        while(sPosition.size()<(sPosSize-1)) {
            sPosition+=' ';
        }
        sPosition+=']';
    }

    m_pRealDevice->Seek(m_sBeaconPos-sPosition.size()-9);
    char ch;
    size_t offset = m_pRealDevice->Tell();
	size_t size;

    /* Sanity tests... */
    size = m_pRealDevice->Read(&ch, 1);
    PODOFO_RAISE_LOGIC_IF( size != 1, "Failed to read 1 byte." );
    if (ch == '0') {
       /* probably clean write mode, whic means two more bytes back */
       m_pRealDevice->Seek(m_sBeaconPos-sPosition.size()-11);
       offset = m_pRealDevice->Tell();
       size = m_pRealDevice->Read(&ch, 1);
       PODOFO_RAISE_LOGIC_IF( size != 1, "Failed to read 1 byte." );
    }

    /* ...the file position should be at the '[' now */
    PODOFO_RAISE_LOGIC_IF( ch != '[', "Failed to find byte range array start in the stream." );

    m_pRealDevice->Seek(offset);
    m_pRealDevice->Write(sPosition.c_str(), sPosition.size());
}

size_t PdfSignOutputDevice::ReadForSignature(char* pBuffer, size_t lLen)
{
    if(!m_bBeaconFound) {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }
	size_t pos = m_pRealDevice->Tell();
	size_t numRead = 0;
	// Check if we are before beacon
	if(pos<m_sBeaconPos)
	{
		size_t readSize = PODOFO_MIN(lLen, m_sBeaconPos-pos);
		if(readSize>0) {
			numRead = m_pRealDevice->Read(pBuffer, readSize);
			pBuffer += numRead;
			lLen -= numRead;
			if(lLen==0) return numRead;
		}
	}
    // shift at the end of beacon
    if ( (pos + numRead) >= m_sBeaconPos && 
        pos < ( m_sBeaconPos + (m_pSignatureBeacon->data().size() + 2) )
    ) {
        m_pRealDevice->Seek( m_sBeaconPos + (m_pSignatureBeacon->data().size() + 2) );
    }
	// read after beacon
	lLen = PODOFO_MIN(lLen, m_pRealDevice->GetLength()-m_pRealDevice->Tell());
	if(lLen==0) return numRead;
	return numRead+m_pRealDevice->Read(pBuffer, lLen);
}

void PdfSignOutputDevice::Write( const char* pBuffer, size_t lLen )
{
    // Check if data with beacon
    if(m_pSignatureBeacon != NULL)
    {
        const std::string & data = m_pSignatureBeacon->data();
        if(data.size() <= lLen)
        {
            const char *pStart = pBuffer;
            const char *pStop = pStart + (lLen-data.size());
            for(; pStart<=pStop; pStart++) {
                if(memcmp(pStart, data.c_str(), data.size())==0)
                {
                    // beacon found
                    m_sBeaconPos = Tell();
                    m_sBeaconPos += (pStart - pBuffer - 1);
                    m_bBeaconFound = true;
                }
            }
        }	
    }
    m_pRealDevice->Write(pBuffer, lLen);
}

}

