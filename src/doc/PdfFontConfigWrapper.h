/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
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

#ifndef _PDF_FONT_CONFIG_WRAPPER_H_
#define _PDF_FONT_CONFIG_WRAPPER_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/Pdf3rdPtyForwardDecl.h"
#include "podofo/base/util/PdfMutex.h"

namespace PoDoFo {

/**
 * This class initializes and destroys the FontConfig library.
 * 
 * As initializing fontconfig can take a long time, you 
 * can create a wrapper by yourself to cache initialization of
 * fontconfig.
 *
 * This class is reference counted. The last user of the fontconfig library
 * will destroy the fontconfig handle.
 *
 * The fontconfig library is initialized on first used (lazy loading!)
 */
class PODOFO_DOC_API PdfFontConfigWrapper {
public:
    /**
     * Create a new FontConfigWrapper and initialize the fontconfig library.
     */
    PdfFontConfigWrapper();

    /**
     * Copy an existing PdfFontConfigWrapper
     */
    PdfFontConfigWrapper(const PdfFontConfigWrapper & rhs);

    ~PdfFontConfigWrapper();

    /**
     * Get access to the internal fontconfig handle.
     * Accesses to this handle have to be synchronized using a mutex!
     *
     * \see GetFontConfigMutex
     *
     * \returns a FcConfig handle (you can cast to FcConfig)
     */
    inline void * GetFontConfig();

    /**
     * Mutex which has to be used to synchronize uses of FontConfig
     */
    inline Util::PdfMutex & GetFontConfigMutex();

    const PdfFontConfigWrapper & operator=(const PdfFontConfigWrapper & rhs);

private:
    /**
     * Destroy fontconfig reference if reference count is 0
     */
    void DerefBuffer();

    /**
     * Do the lazy initialization of fontconfig
     */
    void InitializeFontConfig();

private:

#if defined(PODOFO_HAVE_FONTCONFIG)
    static Util::PdfMutex m_FcMutex;
#endif

    struct TRefCountedFontConfig {
        void* m_pFcConfig;             ///< Handle to fontconfig on unix systems
        long  m_lRefCount;
        bool  m_bInitialized;          ///< Is fontconfig initialized yet?
    };

    TRefCountedFontConfig* m_pFontConfig;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void* PdfFontConfigWrapper::GetFontConfig() 
{
    InitializeFontConfig();
    return m_pFontConfig->m_pFcConfig;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
Util::PdfMutex & PdfFontConfigWrapper::GetFontConfigMutex()
{
    return m_FcMutex;
}

}; // PoDoFo

#endif // _PDF_FONT_WRAPPER_H_
