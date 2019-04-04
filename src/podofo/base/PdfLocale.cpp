/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfDefines.h"
#include "PdfLocale.h"
#include "PdfError.h"
#include "PdfDefinesPrivate.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace PoDoFo {

void PdfLocaleImbue(std::ios_base& s)
{
#if USE_CXX_LOCALE
    static const std::locale cachedLocale( PdfIOLocale );
    try {
    	s.imbue( cachedLocale );
    } catch (const std::runtime_error & e) {
        std::ostringstream err;
        err << "Failed to set safe locale on stream being used for PDF I/O.";
        err << "Locale set was: \"" << PdfIOLocale << "\".";
        err << "Error reported by STL std::locale: \"" << e.what() << "\"";
        // The info string is copied by PdfError so we're ok to just:
        PODOFO_RAISE_ERROR_INFO(
            ePdfError_InvalidDeviceOperation,
            err.str().c_str()
            );
    }
#endif
}

};
