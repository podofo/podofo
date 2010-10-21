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

#ifndef _PDF_ENCODING_FACTORY_H_
#define _PDF_ENCODING_FACTORY_H_

#include "PdfDefines.h"
#include "util/PdfMutex.h"
#include "string.h"

namespace PoDoFo {

class PdfEncoding;
class PdfDocEncoding;
class PdfMacRomanEncoding;
class PdfObject;
class PdfWinAnsiEncoding;
class PdfStandardEncoding; // OC 13.08.2010
class PdfMacExpertEncoding; // OC 13.08.2010
class PdfSymbolEncoding; // OC 13.08.2010
class PdfZapfDingbatsEncoding; // OC 13.08.2010

/** This factory creates a PdfEncoding
 *  from an existing object in the PDF.
 */
class PODOFO_API PdfEncodingFactory {
 public:
    /** Singleton method which returns a global instance
     *  of PdfDocEncoding.
     *
     *  \returns global instance of PdfDocEncoding
     */
    static const PdfEncoding* GlobalPdfDocEncodingInstance();

    /** Singleton method which returns a global instance
     *  of WinAnsiEncoding.
     *
     *  \returns global instance of WinAnsiEncoding
     */
    static const PdfEncoding* GlobalWinAnsiEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacRomanEncoding.
     *
     *  \returns global instance of MacRomanEncoding
     */
    static const PdfEncoding* GlobalMacRomanEncodingInstance();

    // OC 13.08.2010:
    /** Singleton method which returns a global instance
     *  of StandardEncoding.
     *
     *  \returns global instance of StandardEncoding
     */
    static const PdfEncoding* GlobalStandardEncodingInstance();

    // OC 13.08.2010:
    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static const PdfEncoding* GlobalMacExpertEncodingInstance();

    // OC 13.08.2010:
    /** Singleton method which returns a global instance
     *  of SymbolEncoding.
     *
     *  \returns global instance of SymbolEncoding
     */
    static const PdfEncoding* GlobalSymbolEncodingInstance();

    // OC 13.08.2010:
    /** Singleton method which returns a global instance
     *  of ZapfDingbatsEncoding.
     *
     *  \returns global instance of ZapfDingbatsEncoding
     */
    static const PdfEncoding* GlobalZapfDingbatsEncodingInstance();

    /** Free's the memory allocated by
     *  the global encoding instancess in this singleton.
     *
     *  PoDoFo will reallocated these encodings as soon
     *  as they are needed again.
     *
     +  Only call this method if no other class
     *  of PoDoFo exists anymore, as PdfString etc
     *  contain pointers to the global instances.
     */
    static void FreeGlobalEncodingInstances();

    static void PoDoFoClientAttached();

 private:
    /** Always use this static declaration,
     *  if you need an instance of PdfDocEncoding
     *  as heap allocation is expensive for PdfDocEncoding.
     */
    static const PdfDocEncoding* s_pDocEncoding;

    /** Always use this static declaration,
     *  if you need an instance of PdfWinAnsiEncoding
     *  as heap allocation is expensive for PdfWinAnsiEncoding.
     */
    static const PdfWinAnsiEncoding* s_pWinAnsiEncoding;

    /** Always use this static declaration,
     *  if you need an instance of PdfWinAnsiEncoding
     *  as heap allocation is expensive for PdfWinAnsiEncoding.
     */
    static const PdfMacRomanEncoding* s_pMacRomanEncoding;

    // OC 13.08.2010:
    /** Always use this static declaration,
     *  if you need an instance of StandardEncoding
     *  as heap allocation is expensive for PdfStandardEncoding.
     */
    static const PdfStandardEncoding* s_pStandardEncoding;

    // OC 13.08.2010:
    /** Always use this static declaration,
     *  if you need an instance of MacExpertEncoding
     *  as heap allocation is expensive for PdfMacExpertEncoding.
     */
    static const PdfMacExpertEncoding* s_pMacExpertEncoding;

    // OC 13.08.2010:
    /** Always use this static declaration,
     *  if you need an instance of SymbolEncoding
     *  as heap allocation is expensive for PdfSymbolEncoding.
     */
    static const PdfSymbolEncoding* s_pSymbolEncoding;

    // OC 13.08.2010:
    /** Always use this static declaration,
     *  if you need an instance of ZapfDingbatsEncoding
     *  as heap allocation is expensive for PdfZapfDingbatsEncoding.
     */
    static const PdfZapfDingbatsEncoding* s_pZapfDingbatsEncoding;

    static Util::PdfMutex s_mutex;
};

}; /* namespace PoDoFo */


#endif // _PDF_ENCODING_FACTORY_H__

