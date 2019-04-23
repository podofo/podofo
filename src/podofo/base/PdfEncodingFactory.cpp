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

#include "PdfEncodingFactory.h"

#include "PdfEncoding.h"
#include "util/PdfMutexWrapper.h"
#include "PdfName.h"
#include "PdfObject.h"
#include "PdfDefinesPrivate.h"
#include "doc/PdfIdentityEncoding.h"

namespace PoDoFo {

const PdfDocEncoding*      PdfEncodingFactory::s_pDocEncoding      = NULL;
const PdfWinAnsiEncoding*  PdfEncodingFactory::s_pWinAnsiEncoding  = NULL;
const PdfMacRomanEncoding* PdfEncodingFactory::s_pMacRomanEncoding = NULL;
const PdfStandardEncoding*     PdfEncodingFactory::s_pStandardEncoding     = NULL; // OC 13.08.2010 New.
const PdfMacExpertEncoding*    PdfEncodingFactory::s_pMacExpertEncoding    = NULL; // OC 13.08.2010 New.
const PdfSymbolEncoding*       PdfEncodingFactory::s_pSymbolEncoding       = NULL; // OC 13.08.2010 New.
const PdfZapfDingbatsEncoding* PdfEncodingFactory::s_pZapfDingbatsEncoding = NULL; // OC 13.08.2010 New.
const PdfIdentityEncoding *    PdfEncodingFactory::s_pIdentityEncoding = NULL;
const PdfWin1250Encoding *     PdfEncodingFactory::s_pWin1250Encoding = NULL;
const PdfIso88592Encoding *    PdfEncodingFactory::s_pIso88592Encoding = NULL;

Util::PdfMutex PdfEncodingFactory::s_mutex;

PdfEncodingFactory::PdfEncodingFactory()
{
}
	
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

const PdfEncoding* PdfEncodingFactory::GlobalIdentityEncodingInstance()
{
    if(!s_pIdentityEncoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pIdentityEncoding) // Double check
            s_pIdentityEncoding = new PdfIdentityEncoding( 0, 0xffff, false );
    }

    return s_pIdentityEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalWin1250EncodingInstance()
{
    if(!s_pWin1250Encoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pWin1250Encoding) // Double check
            s_pWin1250Encoding = new PdfWin1250Encoding();
    }

    return s_pWin1250Encoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalIso88592EncodingInstance()
{
    if(!s_pIso88592Encoding) // First check
    {
        Util::PdfMutexWrapper wrapper( PdfEncodingFactory::s_mutex ); 
        
        if(!s_pIso88592Encoding) // Double check
            s_pIso88592Encoding = new PdfIso88592Encoding();
    }

    return s_pIso88592Encoding;
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
        if (NULL != s_pIdentityEncoding)
        {
            delete s_pIdentityEncoding;
        }
        if (NULL != s_pWin1250Encoding)
        {
            delete s_pWin1250Encoding;
        }
        if (NULL != s_pIso88592Encoding)
        {
            delete s_pIso88592Encoding;
        }

        s_pMacRomanEncoding     = NULL;
        s_pWinAnsiEncoding      = NULL;
        s_pDocEncoding          = NULL;
        s_pStandardEncoding     = NULL; // OC 13.08.2010
        s_pMacExpertEncoding    = NULL; // OC 13.08.2010
        s_pSymbolEncoding       = NULL; // OC 13.08.2010
        s_pZapfDingbatsEncoding = NULL; // OC 13.08.2010
        s_pIdentityEncoding     = NULL;
        s_pWin1250Encoding      = NULL;
        s_pIso88592Encoding     = NULL;
    }
}

void PdfEncodingFactory::PoDoFoClientAttached()
{
    podofo_number_of_clients++;
}

};
