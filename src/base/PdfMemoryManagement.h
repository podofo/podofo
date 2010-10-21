/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

#ifndef _PDF_MEMORY_MANAGEMENT_H_
#define _PDF_MEMORY_MANAGEMENT_H_

#include "PdfDefines.h"
#include <stdlib.h>

namespace PoDoFo {

/**
 * Wrapper around malloc of the c-library used by PoDoFo.
 *
 * Is used to allocate buffers inside of PoDoFo.
 */
PODOFO_API void* podofo_malloc( size_t size );

/**
 * Wrapper around realloc of the c-library used by PoDoFo.
 */
PODOFO_API void* podofo_realloc( void* buffer, size_t size );

/**
 * Wrapper around free of the c-library used by PoDoFo.
 *
 * Use this to free memory allocated inside of PoDoFo
 * with podofo_malloc.
 */
PODOFO_API void podofo_free( void* buffer );

/**
 * Check during runtime if the current architecture is big- or little-endian.
 * \returns true if the architecture is little-endian
 */
PODOFO_API bool podofo_is_little_endian();


};

#endif // _PDF_XREF_STREAM_PARSER_OBJECT_H_

