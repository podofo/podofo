/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "PdfEncodingFactory.h"

#include "PdfEncoding.h"
#include "util/PdfMutexWrapper.h"
#include "PdfName.h"
#include "PdfObject.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

const PdfDocEncoding*      PdfEncodingFactory::s_pDocEncoding      = NULL;
const PdfWinAnsiEncoding*  PdfEncodingFactory::s_pWinAnsiEncoding  = NULL;
const PdfMacRomanEncoding* PdfEncodingFactory::s_pMacRomanEncoding = NULL;
const PdfStandardEncoding*     PdfEncodingFactory::s_pStandardEncoding     = NULL; // OC 13.08.2010 New.
const PdfMacExpertEncoding*    PdfEncodingFactory::s_pMacExpertEncoding    = NULL; // OC 13.08.2010 New.
const PdfSymbolEncoding*       PdfEncodingFactory::s_pSymbolEncoding       = NULL; // OC 13.08.2010 New.
const PdfZapfDingbatsEncoding* PdfEncodingFactory::s_pZapfDingbatsEncoding = NULL; // OC 13.08.2010 New.

Util::PdfMutex PdfEncodingFactory::s_mutex;

const PdfEncoding* PdfEncodingFactory::GlobalPdfDocEncodingInstance()
{
    if(!s_pDocEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pDocEncoding) // Double check
            s_pDocEncoding = new PdfDocEncoding();
    }

    return s_pDocEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalWinAnsiEncodingInstance()
{
    if(!s_pWinAnsiEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pWinAnsiEncoding) // Double check
            s_pWinAnsiEncoding = new PdfWinAnsiEncoding();
    }

    return s_pWinAnsiEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalMacRomanEncodingInstance()
{
    if(!s_pMacRomanEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pMacRomanEncoding) // Double check
            s_pMacRomanEncoding = new PdfMacRomanEncoding();
    }

    return s_pMacRomanEncoding;
}

// OC 13.08.2010:
const PdfEncoding* PdfEncodingFactory::GlobalStandardEncodingInstance()
{
    if(!s_pStandardEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pStandardEncoding) // Double check
            s_pStandardEncoding = new PdfStandardEncoding();
    }
    
    return s_pStandardEncoding;
}

// OC 13.08.2010:
const PdfEncoding* PdfEncodingFactory::GlobalMacExpertEncodingInstance()
{
    if(!s_pMacExpertEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pMacExpertEncoding) // Double check
            s_pMacExpertEncoding = new PdfMacExpertEncoding();
    }
    
    return s_pMacExpertEncoding;
}

// OC 13.08.2010:
const PdfEncoding* PdfEncodingFactory::GlobalSymbolEncodingInstance()
{
    if(!s_pSymbolEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pSymbolEncoding) // Double check
            s_pSymbolEncoding = new PdfSymbolEncoding();
    }

    return s_pSymbolEncoding;
}

// OC 13.08.2010:
const PdfEncoding* PdfEncodingFactory::GlobalZapfDingbatsEncodingInstance()
{
    if(!s_pZapfDingbatsEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pZapfDingbatsEncoding) // Double check
            s_pZapfDingbatsEncoding = new PdfZapfDingbatsEncoding();
    }
    
    return s_pZapfDingbatsEncoding;
}


int podofo_number_of_clients = 0;

void PdfEncodingFactory::FreeGlobalEncodingInstances()
{
    Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 	
    
    podofo_number_of_clients--;
    if (podofo_number_of_clients <= 0)
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if (NULL != s_pMacRomanEncoding)
        {
            delete s_pMacRomanEncoding;
        }
        if (NULL != s_pWinAnsiEncoding)
        {
            delete s_pWinAnsiEncoding;
        }
        if (NULL != s_pDocEncoding)
        {
            delete s_pDocEncoding;
        }
        if (NULL != s_pStandardEncoding) // OC 13.08.2010
        {
            delete s_pStandardEncoding;
        }
        if (NULL != s_pMacExpertEncoding) // OC 13.08.2010
        {
            delete s_pMacExpertEncoding;
        }
        if (NULL != s_pSymbolEncoding) // OC 13.08.2010
        {
            delete s_pSymbolEncoding;
        }
        if (NULL != s_pZapfDingbatsEncoding) // OC 13.08.2010
        {
            delete s_pZapfDingbatsEncoding;
        }

        s_pMacRomanEncoding     = NULL;
        s_pWinAnsiEncoding      = NULL;
        s_pDocEncoding          = NULL;
        s_pStandardEncoding     = NULL; // OC 13.08.2010
        s_pMacExpertEncoding    = NULL; // OC 13.08.2010
        s_pSymbolEncoding       = NULL; // OC 13.08.2010
        s_pZapfDingbatsEncoding = NULL; // OC 13.08.2010
    }
}

void PdfEncodingFactory::PoDoFoClientAttached()
{
    podofo_number_of_clients++;
}

};
