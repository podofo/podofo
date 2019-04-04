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

#include "PdfFontMetrics.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfVariant.h"

#include "PdfFontFactory.h"

#include <sstream>

#include <wchar.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#define PODOFO_FIRST_READABLE 31
#define PODOFO_WIDTH_CACHE_SIZE 256

namespace PoDoFo {

#if defined(__APPLE_CC__) && !defined(PODOFO_HAVE_FONTCONFIG)
#include <Carbon/Carbon.h>
#endif
PdfFontMetrics::PdfFontMetrics( EPdfFontType eFontType, const char* pszFilename, const char* pszSubsetPrefix )

    : m_sFilename( pszFilename ),
      m_fFontSize( 0.0f ),
      m_fFontScale( 100.0f ),
      m_fFontCharSpace( 0.0f ),
      m_fWordSpace( 0.0f ),
      m_eFontType( eFontType ),
      m_sFontSubsetPrefix( pszSubsetPrefix ? pszSubsetPrefix : "" )
{

}
/*
PdfFontMetrics::PdfFontMetrics( FT_Library* pLibrary, PdfObject* pDescriptor )
    : m_sFilename( "" ), m_pLibrary( pLibrary ), m_pMetrics_base14(NULL),
      m_bSymbol( false ), m_fFontSize( 0.0f ),
      m_fFontScale( 100.0f ), m_fFontCharSpace( 0.0f ),
      m_eFontType( ePdfFontType_Unknown )
{
    m_face = NULL;

    if( !pDescriptor )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfName sName  = pDescriptor->GetDictionary().GetKey( "FontName" )->GetName();
    m_nWeight      = static_cast<unsigned int>(pDescriptor->GetDictionary().GetKeyAsLong( "FontWeight", 400L ));
    m_nItalicAngle = static_cast<int>(pDescriptor->GetDictionary().GetKeyAsLong( "ItalicAngle", 0L ));

    m_dPdfAscent   = pDescriptor->GetDictionary().GetKeyAsReal( "Ascent", 0.0 );
    m_dPdfDescent  = pDescriptor->GetDictionary().GetKeyAsReal( "Descent", 0.0 );
}
*/

PdfFontMetrics::~PdfFontMetrics()
{
}

#if defined(__APPLE_CC__) && !defined(PODOFO_HAVE_FONTCONFIG) && !defined(PODOFO_NO_FONTMANAGER)
FT_Error
My_FT_GetFile_From_Mac_ATS_Name( const char*  fontName,
                                 FSSpec*  pathSpec, FT_Long*     face_index )
{
    CFStringRef  cf_fontName;
    ATSFontRef   ats_font_id;

    *face_index = 0;

    cf_fontName = CFStringCreateWithCString( NULL, fontName, kCFStringEncodingMacRoman );
    ats_font_id = ATSFontFindFromName( cf_fontName, kATSOptionFlagsUnRestrictedScope );

    if ( ats_font_id == 0 || ats_font_id == 0xFFFFFFFFUL )
        return FT_Err_Unknown_File_Format;

    if ( 0 != ATSFontGetFileSpecification( ats_font_id, pathSpec ) )
        return FT_Err_Unknown_File_Format;

    /* face_index calculation by searching preceding fontIDs */
    /* with same FSRef                                       */
    {
        int     i;
        FSSpec  f;


        for ( i = 1; i < ats_font_id; i++ )
        {
            if ( 0 != ATSFontGetFileSpecification( ats_font_id - i, &f ) ||
                f.vRefNum != pathSpec->vRefNum                       ||
                f.parID   != pathSpec->parID                         ||
                f.name[0] != pathSpec->name[0]                       ||
                0 != ft_strncmp( (char *)f.name + 1,
                (char *)pathSpec->name + 1,
                f.name[0]                           ) )
                break;
        }
        *face_index = ( i - 1 );
    }
    return FT_Err_Ok;
}

  FT_Error
  My_FT_GetFile_From_Mac_Name( const char* fontName,
                            FSSpec*     pathSpec,
                            FT_Long*    face_index )
  {
    OptionBits            options = kFMUseGlobalScopeOption;

    FMFontFamilyIterator  famIter;
    OSStatus              status = FMCreateFontFamilyIterator( NULL, NULL,
                                                               options,
                                                               &famIter );
    FMFont                the_font = NULL;
    FMFontFamily          family   = NULL;


    *face_index = 0;
    while ( status == 0 && !the_font )
    {
      status = FMGetNextFontFamily( &famIter, &family );
      if ( status == 0 )
      {
        int                           stat2;
        FMFontFamilyInstanceIterator  instIter;
        Str255                        famNameStr;
        char                          famName[256];


        /* get the family name */
        FMGetFontFamilyName( family, famNameStr );
( famNameStr, famName );

        // inLog.Debug( boost::format( "Found FontFamily: '%s'\n" ) % famName );

        /* iterate through the styles */
        FMCreateFontFamilyInstanceIterator( family, &instIter );

        *face_index = 0;
        stat2 = 0;
        while ( stat2 == 0 && !the_font )
        {
          FMFontStyle  style;
          FMFontSize   size;
          FMFont       font;


          stat2 = FMGetNextFontFamilyInstance( &instIter, &font,
                                               &style, &size );
          if ( stat2 == 0 && size == 0 )
          {
            char  fullName[256];


            /* build up a complete face name */
            ft_strcpy( fullName, famName );
            if ( style & bold )
              strcat( fullName, " Bold" );
            if ( style & italic )
              strcat( fullName, " Italic" );

            // inLog.Debug( boost::format( "Checking Face: '%s'\n" ) % fullName );

            /* compare with the name we are looking for */
            if ( ft_strcmp( fullName, fontName ) == 0 )
            {
              /* found it! */
              the_font = font;
            }
            else
              ++(*face_index);
          }
        }

        FMDisposeFontFamilyInstanceIterator( &instIter );
      }
    }

    FMDisposeFontFamilyIterator( &famIter );

    if ( the_font )
    {
      FMGetFontContainer( the_font, pathSpec );
      return FT_Err_Ok;
    }
    else
      return FT_Err_Unknown_File_Format;
  }

/* Given a PostScript font name, create the Macintosh LWFN file name. */
  static void
  create_lwfn_name( char*   ps_name,
                    Str255  lwfn_file_name )
  {
    int       max = 5, count = 0;
    FT_Byte*  p = lwfn_file_name;
    FT_Byte*  q = (FT_Byte*)ps_name;


    lwfn_file_name[0] = 0;

    while ( *q )
    {
      if ( ft_isupper( *q ) )
      {
        if ( count )
          max = 3;
        count = 0;
      }
      if ( count < max && ( ft_isalnum( *q ) || *q == '_' ) )
      {
        *++p = *q;
        lwfn_file_name[0]++;
        count++;
      }
      q++;
    }
  }

  static short
  count_faces_sfnt( char *fond_data )
  {
    /* The count is 1 greater than the value in the FOND.  */
    /* Isn't that cute? :-)                                */

    return 1 + *( (short *)( fond_data + sizeof ( FamRec ) ) );
  }

static void
  parse_fond( char*   fond_data,
              short*  have_sfnt,
              short*  sfnt_id,
              char*   ps_name,
              Str255  lwfn_file_name,
              short   face_index )
  {
    AsscEntry*  assoc;
    AsscEntry*  base_assoc;
    FamRec*     fond;


    *sfnt_id          = 0;
    *have_sfnt        = 0;
    lwfn_file_name[0] = 0;

    fond       = (FamRec*)fond_data;
    assoc      = (AsscEntry*)( fond_data + sizeof ( FamRec ) + 2 );
    base_assoc = assoc;

    /* Let's do a little range checking before we get too excited here */
    if ( face_index < count_faces_sfnt( fond_data ) )
    {
      assoc += face_index;        /* add on the face_index! */

      /* if the face at this index is not scalable,
         fall back to the first one (old behavior) */
      if ( assoc->fontSize == 0 )
      {
        *have_sfnt = 1;
        *sfnt_id   = assoc->fontID;
      }
      else if ( base_assoc->fontSize == 0 )
      {
        *have_sfnt = 1;
        *sfnt_id   = base_assoc->fontID;
      }
    }

    if ( fond->ffStylOff )
    {
      unsigned char*  p = (unsigned char*)fond_data;
      StyleTable*     style;
      unsigned short  string_count;
      unsigned char*  names[64];
      int             i;

      // inLog.Debug( "Font has StylOff\n" );

      p += fond->ffStylOff;
      style = (StyleTable*)p;
      p += sizeof ( StyleTable );
      string_count = *(unsigned short*)(p);
      p += sizeof ( short );

      for ( i = 0 ; i < string_count && i < 64; i++ )
      {
        names[i] = p;
        p += names[i][0];
        p++;
// 		inLog.Debug( boost::format( "Name[%d] is '%s'\n" ) % i % &names[i][1] );
     }

      {
        size_t  ps_name_len = (size_t)names[0][0];

        if ( ps_name_len != 0 )
        {
          ft_memcpy(ps_name, names[0] + 1, ps_name_len);
          ps_name[ps_name_len] = 0;
        }
        if ( style->indexes[0] > 1 )
        {
          unsigned char*  suffixes = names[style->indexes[0] - 1];

// 		  inLog.Debug( boost::format( "string_count = %d\tsuffixes = %d\n" ) % string_count % (int)suffixes[0] );

          for ( i = 1; i <= suffixes[0]; i++ )
          {
            unsigned char*  s;
            size_t          j = suffixes[i] - 1;


            if ( j < string_count && ( s = names[j] ) != NULL )
            {
              size_t  s_len = (size_t)s[0];
              s[s_len] = 0;

// 			  inLog.Debug( boost::format( "Suffix %d:'%s'\n" ) % i % &s[1] );

              if ( s_len != 0 && ps_name_len + s_len < sizeof ( ps_name ) )
              {
                ft_memcpy( ps_name + ps_name_len, s + 1, s_len );
                ps_name_len += s_len;
                ps_name[ps_name_len] = 0;
              }
            }
          }
        }
      }

      // inLog.Debug( boost::format( "Found PSName is '%s'\n" ) % ps_name );
      create_lwfn_name( ps_name, lwfn_file_name );
    }
  }

/* Given a file reference, answer its location as a vRefNum
     and a dirID. */
  static FT_Error
  get_file_location( short           ref_num,
                     short*          v_ref_num,
                     long*           dir_id,
                     unsigned char*  file_name )
  {
    FCBPBRec  pb;
    OSErr     error;


    pb.ioNamePtr = file_name;
    pb.ioVRefNum = 0;
    pb.ioRefNum  = ref_num;
    pb.ioFCBIndx = 0;

    error = PBGetFCBInfoSync( &pb );
    if ( error == noErr )
    {
      *v_ref_num = pb.ioFCBVRefNum;
      *dir_id    = pb.ioFCBParID;
    }
    return error;
  }

  /* Return the file type of the file specified by spec. */
  static OSType get_file_type( const FSSpec*  spec )
  {
      FInfo  finfo;


      if ( FSpGetFInfo( spec, &finfo ) != noErr )
          return 0;  /* file might not exist */

      return finfo.fdType;
  }

  /* Make a file spec for an LWFN file from a FOND resource and
     a file name. */
  static FT_Error
  make_lwfn_spec( Handle               fond,
                  const unsigned char* file_name,
                  FSSpec*              spec )
  {
    FT_Error  error;
    short     ref_num, v_ref_num;
    long      dir_id;
    Str255    fond_file_name;


    ref_num = HomeResFile( fond );

    error = ResError();
    if ( !error )
      error = get_file_location( ref_num, &v_ref_num,
                                 &dir_id, fond_file_name );
    if ( !error )
      error = FSMakeFSSpec( v_ref_num, dir_id, file_name, spec );

    return error;
  }

 /* Read Type 1 data from the POST resources inside the LWFN file,
     return a PFB buffer. This is somewhat convoluted because the FT2
     PFB parser wants the ASCII header as one chunk, and the LWFN
     chunks are often not organized that way, so we'll glue chunks
     of the same type together. */
  static FT_Error
  read_lwfn( short      res_ref,
             FT_Byte**  pfb_data,
             FT_ULong*  size )
  {
    FT_Error       error = FT_Err_Ok;
    short          res_id;
    unsigned char  *buffer, *p, *size_p = NULL;
    FT_ULong       total_size = 0;
    FT_ULong       post_size, pfb_chunk_size;
    Handle         post_data;
    char           code, last_code;


    UseResFile( res_ref );

    /* First pass: load all POST resources, and determine the size of */
    /* the output buffer.                                             */
    res_id    = 501;
    last_code = -1;

    for (;;)
    {
      post_data = Get1Resource( 'POST', res_id++ );
      if ( post_data == NULL )
        break;  /* we're done */

      code = (*post_data)[0];

      if ( code != last_code )
      {
        if ( code == 5 )
          total_size += 2; /* just the end code */
        else
          total_size += 6; /* code + 4 bytes chunk length */
      }

      total_size += GetHandleSize( post_data ) - 2;
      last_code = code;
    }

    buffer = (unsigned char*)podofo_malloc( total_size );
    if ( buffer == NULL )
      goto Error;

    /* Second pass: append all POST data to the buffer, add PFB fields. */
    /* Glue all consecutive chunks of the same type together.           */
    p              = buffer;
    res_id         = 501;
    last_code      = -1;
    pfb_chunk_size = 0;

    for (;;)
    {
      post_data = Get1Resource( 'POST', res_id++ );
      if ( post_data == NULL )
        break;  /* we're done */

      post_size = (FT_ULong)GetHandleSize( post_data ) - 2;
      code = (*post_data)[0];

      if ( code != last_code )
      {
        if ( last_code != -1 )
        {
          /* we're done adding a chunk, fill in the size field */
          if ( size_p != NULL )
          {
            *size_p++ = (FT_Byte)(   pfb_chunk_size         & 0xFF );
            *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 8  ) & 0xFF );
            *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 16 ) & 0xFF );
            *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 24 ) & 0xFF );
          }
          pfb_chunk_size = 0;
        }

        *p++ = 0x80;
        if ( code == 5 )
          *p++ = 0x03;  /* the end */
        else if ( code == 2 )
          *p++ = 0x02;  /* binary segment */
        else
          *p++ = 0x01;  /* ASCII segment */

        if ( code != 5 )
        {
          size_p = p;   /* save for later */
          p += 4;       /* make space for size field */
        }
      }

      ft_memcpy( p, *post_data + 2, post_size );
      pfb_chunk_size += post_size;
      p += post_size;
      last_code = code;
    }

    *pfb_data = buffer;
    *size = total_size;

  Error:
    CloseResFile( res_ref );
    return error;
  }

static short count_faces( Handle  fond )
  {
    short   sfnt_id, have_sfnt, have_lwfn = 0;
    Str255  lwfn_file_name;
    FSSpec  lwfn_spec;
    char	ps_name[256];

    HLock( fond );
    parse_fond( *fond, &have_sfnt, &sfnt_id, ps_name, lwfn_file_name, 0 );
    HUnlock( fond );

    if ( lwfn_file_name[0] )
    {
      if ( make_lwfn_spec( fond, lwfn_file_name, &lwfn_spec ) == FT_Err_Ok )
        have_lwfn = 1;  /* yeah, we got one! */
      else
        have_lwfn = 0;  /* no LWFN file found */
    }

    if ( have_lwfn && ( !have_sfnt /*|| PREFER_LWFN*/ ) )
      return 1;
    else
      return count_faces_sfnt( *fond );
  }

static FT_Error LoadFontFromLWFN( FSRef inFileRef, FSSpec inSpec,
                                    const char* inFontName, FT_Long inFaceIndex,
                                    char** outBuffer, long& outBufLen )
{
        FT_Error  error = FT_Err_Ok;
        short     res_ref;
        FT_Byte*  pfb_data;
        FT_ULong  pfb_size;

        // open up the resource file
        error = FSOpenResourceFile( &inFileRef, 0, NULL, fsRdPerm, &res_ref );
        if ( error != noErr ) {
            // try old fashioned way
            // inLog.Debug( boost::format( "FSOpenResourceFile failed - Error %d\n" ) % error );

            res_ref = FSpOpenResFile( &inSpec, fsRdPerm );
            if ( res_ref < 0 ) {
                // inLog.Debug( boost::format( "FSpOpenResFile failed- Error %d\n" ) % res_ref );
                return FT_Err_Cannot_Open_Resource;
            } else {
                // inLog.Debug( "FSpOpenResFile Succeeded!\n" );
            }
            error = 0;	// reset it
        }
        UseResFile( res_ref );

        error = read_lwfn( res_ref, &pfb_data, &pfb_size );
        if ( !error ) {
            *outBuffer = (char*)pfb_data;
            outBufLen = pfb_size;
        } else {
            // inLog.Debug( "read_lwfn failed\n" );
        }

Error:
      CloseResFile( res_ref );
      return error;
  }


  static FT_Error LoadFontFromDFont( FSRef inFileRef, FSSpec inSpec,
                                    const char* inFontName, FT_Long inFaceIndex,
                                    char** outBuffer, long& outBufLen )
{
    const bool PREFER_LWFN=false;
    FT_Error  error = FT_Err_Ok;
    short     res_ref, res_index = 1;
    Handle    fond;
    short   sfnt_id = 0, have_sfnt =0, have_lwfn = 0;
    short	num_faces;
    char	ps_name[128];
    Str255  lwfn_file_name;
    FSSpec  lwfn_spec;

    char	localFontName[256];

#if 1
    int j = 0;
    bool	foundSpace = false;
    for ( int i=0; i<strlen( inFontName ); i++ ) {
        if ( inFontName[i] == '-' ) {
            if ( !foundSpace ) {
                localFontName[j++] = ' ';
                foundSpace = true;
            } else {
                // do nothing, we skip over it!
            }
        } else {
            localFontName[j++] = inFontName[i];
        }
    }
    localFontName[j] = 0;	// helps to zero term
    // inLog.Debug( boost::format( "LocalFontName: %s\n" ) % localFontName );
#else
    strcpy( localFontName, inFontName );
#endif

    // open up the resource file
    error = FSOpenResourceFile( &inFileRef, 0, NULL, fsRdPerm, &res_ref );
    if ( error != noErr ) {
        // try old fashioned way
        // inLog.Debug( boost::format( "FSOpenResourceFile failed - Error %d\n" ) % error );

        res_ref = FSpOpenResFile( &inSpec, fsRdPerm );
        if ( res_ref < 0 ) {
            // inLog.Debug( boost::format( "FSpOpenResFile failed- Error %d\n" ) % res_ref );
            return FT_Err_Cannot_Open_Resource;
        } else {
            // inLog.Debug( "FSpOpenResFile Succeeded!\n" );
        }
        error = 0;	// reset it
    }
    UseResFile( res_ref );


    int	numFONDs = Count1Resources( 'FOND' );
    // inLog.Debug( boost::format( "Number of 'FOND' resources = %d\n" ) % numFONDs );

   for ( res_index = 1; ; ++res_index )
    {
      fond = Get1IndResource( 'FOND', res_index );
      if ( ResError() ) {
        // inLog.Debug( boost::format( "Get1IndResource ('FOND' #%d) failed - Error %d\n" ) % res_index % ResError() );
        error = FT_Err_Cannot_Open_Resource;
        goto Error;
      }

      short   fond_id;
      OSType  fond_type;
      Str255  fond_name;
      GetResInfo( fond, &fond_id, &fond_type, fond_name );
      if ( ResError() != noErr || fond_type != 'FOND' ) {
          // inLog.Debug( boost::format( "GetResInfo failed - Error %d\n") % ResError() );
          error = FT_Err_Invalid_File_Format;
          goto Error;
      }
      fond_name[ fond_name[0]+1 ] = 0;

      // check to make sure this is a font we want to load (TTF or OTF)
      HLock( fond );
      parse_fond( *fond, &have_sfnt, &sfnt_id, ps_name, lwfn_file_name, inFaceIndex );
      HUnlock( fond );

      // if either the original font name OR the modified one match - go for it!
      // inLog.Debug( boost::format( "FOND name: '%s' - PSName: '%s'\n" ) % &fond_name[1] % ps_name );
      if ( ft_strcmp( (char*)&fond_name[1] /*ps_name*/, localFontName ) == 0 ) {
          inFaceIndex = res_index-1;
          // inLog.Debug( boost::format( "Matched '%s' at res_index:%d\n" ) % &fond_name[1] % res_index );
          break;
      } else if ( ft_strcmp( (char*)&fond_name[1], inFontName ) == 0 ) {
          inFaceIndex = res_index-1;
          // inLog.Debug( boost::format( "Matched '%s' at res_index:%d\n" ) % &fond_name[1] % res_index );
          break;
      }

    }

    if ( lwfn_file_name[0] )
    {
        lwfn_file_name[lwfn_file_name[0]+1] = 0;	// zero term for C
      // inLog.Debug( boost::format( "Found LWFN at '%s'\n" ) % &lwfn_file_name[1] );

      if ( make_lwfn_spec( fond, lwfn_file_name, &lwfn_spec ) == FT_Err_Ok )
        have_lwfn = 1;  /* yeah, we got one! */
      else
        have_lwfn = 0;  /* no LWFN file found */
    }

    if ( have_lwfn && ( !have_sfnt || PREFER_LWFN ) ) {
        FT_Byte*  pfb_data;
        FT_ULong  pfb_size;
        FT_Error  error;
        short     res_ref;

        res_ref = FSpOpenResFile( &lwfn_spec, fsRdPerm );
        if ( res_ref < 0 ) {
            // inLog.Debug( "FSpOpenResFile on LWFN failed\n" );
            return FT_Err_Cannot_Open_Resource;
        } else {
            // inLog.Debug( "FSpOpenResFile on LWFN succeeded!\n" );
        }

        error = read_lwfn( res_ref, &pfb_data, &pfb_size );
        if ( !error ) {
            *outBuffer = (char*)pfb_data;
            outBufLen = pfb_size;
        } else {
            // inLog.Debug( "read_lwfn failed\n" );
        }
    } else if ( have_sfnt ) {
        Handle     sfnt = NULL;
        FT_Byte*   sfnt_data;
        size_t     sfnt_size;

        // inLog.Debug( "Loading from SFNT...\n" );

        sfnt = GetResource( 'sfnt', sfnt_id );
        if ( ResError() ) {
            // inLog.Debug( boost::format( "GetResource ('sfnt' #%d) failed - Error %d\n" ) % sfnt_id % ResError() );
            return FT_Err_Invalid_Handle;
        }

        sfnt_size = (FT_ULong)GetHandleSize( sfnt );
        sfnt_data = (FT_Byte*)ASmalloc( (FT_Long)sfnt_size );
        if ( sfnt_data == NULL ) {
            ReleaseResource( sfnt );
            return error;
        }

        HLock( sfnt );
        memcpy( sfnt_data, *sfnt, sfnt_size );
        HUnlock( sfnt );
        ReleaseResource( sfnt );

        *outBuffer = (char*)sfnt_data;
        outBufLen = sfnt_size;
    } else {
        // inLog.Debug( boost::format( "have_sfnt is false, sfnt_id is %d, and inFaceIndex is %d\n" ) % sfnt_id % inFaceIndex );
    }

Error:
    CloseResFile( res_ref );
    return error;
}

std::string	Std2AltFontName( const std::string& inStdName )
{
    std::string	altName( "" );

    if ( inStdName == "Courier" )
        altName.assign( "Courier New" );
    else if ( inStdName == "Courier-Bold" )
        altName.assign( "Courier New Bold" );
    else if ( inStdName == "Courier-Oblique" )
        altName.assign( "Courier New Italic" );
    else if ( inStdName == "Courier-BoldOblique" )
        altName.assign( "Courier New Bold Italic" );
    else if ( inStdName == "Times-Roman" )
        altName.assign( "Times New Roman" );
    else if ( inStdName == "Times-Bold" )
        altName.assign( "Times New Roman Bold" );
    else if ( inStdName == "Times-Italic" )
        altName.assign( "Times New Roman Italic" );
    else if ( inStdName == "Times-BoldItalic" )
        altName.assign( "Times New Roman Bold Italic" );
    else if ( inStdName == "ZapfDingbats" )
        altName.assign( "Zapf Dingbats" );

    // if we haven't already found it, try doing common subs
    if ( altName.empty() ) {
        int j = 0;
        bool	foundSpace = false;
        for ( int i=0; i<inStdName.length(); i++ ) {
            if ( inStdName[i] == ',' ) {
                altName += ' ';
            } else if ( inStdName[i] == '-' ) {
                if ( !foundSpace ) {
                    altName += ' ';
                    foundSpace = true;
                } else {
                    // do nothing, we skip over it!
                }
            } else {
                altName += inStdName[i];
            }
        }
    }

    return altName;
}

std::string PdfFontMetrics::GetFilenameForFont( const char* pszFontname )
{
    // TODO: Use FT_GetFile_From_Mac_ATS_Name

    FSSpec  fSpec;
    FT_Long fIndex = 0;
    FT_Error    error = My_FT_GetFile_From_Mac_ATS_Name( const_cast<char*>(pszFontname), &fSpec, &fIndex );
    if ( error ) {
        // try use the alternate name...
        std::string	altName = Std2AltFontName( pszFontname );
        // mLog.Debug( boost::format("Unable to locate - trying alternate '%s'\n") % altName.c_str() );
        error = My_FT_GetFile_From_Mac_ATS_Name( const_cast<char*>(altName.c_str()), &fSpec, &fIndex );
        if ( error ) {
            // mLog.Debug( boost::format("Unable to locate - trying as Postscript\n") );

            // see if this is a Postscript name...
            CFStringRef    cstr = CFStringCreateWithCString( NULL, pszFontname, kCFStringEncodingUTF8 );
            if ( cstr != NULL ) {
                ATSFontRef  fontRef = ATSFontFindFromPostScriptName( cstr, kATSOptionFlagsDefault );
                if ( fontRef != kATSFontRefUnspecified ) {
                    // mLog.Debug( "**Found it!\n" );
                    error = ATSFontGetFileSpecification( fontRef, &fSpec );
                } else {
                    // mLog.Debug( boost::format("*Unable to locate as Postscript - giving up!\n") );
                }
                CFRelease( cstr );
            }
        }
    }

    if ( !error ) {
        FSRef	 	ref;
        OSErr   err = FSpMakeFSRef( &fSpec, &ref );
        if ( !err ) {
            CFURLRef    url = CFURLCreateFromFSRef( kCFAllocatorDefault, &ref );
            CFStringRef pathRef = CFURLCopyFileSystemPath( url, kCFURLPOSIXPathStyle );
            CFIndex     length = CFStringGetLength( pathRef ) + 0x02;
            char*       path = (char *)podofo_calloc( length, sizeof( *path ) );
            if ( CFStringGetCString( pathRef, path, length, kCFStringEncodingUTF8 ) ) {
                std::string fontPath( path );
                if ( (fontPath.find( ".ttf" ) != fontPath.npos) || (fontPath.find( ".otf" ) != fontPath.npos) ) {
                    // mLog.Debug( boost::format("Found matching TTF/OTF font for '%s', index %d\n") % pszFontname % fIndex );
#if 1	//def FILE_BASED
                    CPDFFTFont*	ftFont = new CPDFFTFont( *this, fontPath, fIndex );
#else
                    std::string	fontBufStr;
                    StringUtils::ReadFileIntoString( fontPath, fontBufStr );
                    ASInt32	fontBufferLen = fontBufStr.length();
                    if (fontBufferLen == 0) {
                        CFRelease( pathRef );
                        CFRelease( url );
                        podofo_free(path);
                        return NULL;
                    }
                    char*	fontBuffer = (char*)ASmalloc( fontBufferLen );
                    memcpy( fontBuffer, fontBufStr.c_str(), fontBufferLen );

                    CPDFFTFont*	ftFont = new CPDFFTFont( *this, fontBuffer, fontBufferLen, fIndex );
#endif
                    retFont = reinterpret_cast< CPDFFont* >( ftFont );
                } else if ( fontPath.find( ".dfont" ) != fontPath.npos ) {
                    char*	fontBuffer = NULL;
                    ASInt32	fontBufferLen = 0;

                    // mLog.Debug( boost::format("Found a matching .dfont for '%s', index %d\n") % pszFontname % fIndex );
                    FT_Error dfErr = LoadFontFromDFont( mLog, ref, fSpec, pszFontname, fIndex, &fontBuffer, fontBufferLen );
                    if ( !dfErr ) {
                        CPDFFTFont*	ftFont = new CPDFFTFont( *this, fontBuffer, fontBufferLen, fIndex );
                        retFont = reinterpret_cast< CPDFFont* >( ftFont );
                    }
                } else {
                    char*	fontBuffer = NULL;
                    ASInt32	fontBufferLen = 0;

                    fSpec.name[ fSpec.name[0]+1 ] = 0;	// zero term for C func
                    // mLog.Debug( boost::format("Found a matching CLASSIC font for '%s' at '%s', index %d\n") % pszFontname % &fSpec.name[1] % fIndex );
                    FT_Error dfErr = 0;
                    OSType file_type = get_file_type( &fSpec );
                    if ( file_type == 'LWFN' ) {
                        // mLog.Debug( "Loading from LWFN...\n" );
                        if ( fIndex > 0 ) fIndex = 0;	// don't need it anymore...
                        dfErr = LoadFontFromLWFN( mLog, ref, fSpec, pszFontname, fIndex, &fontBuffer, fontBufferLen );
                    } else {
                        // mLog.Debug( "Loading from Suitcase...\n" );
                        dfErr = LoadFontFromDFont( mLog, ref, fSpec, pszFontname, fIndex, &fontBuffer, fontBufferLen );
                    }
                    if ( !dfErr ) {
                        CPDFFTFont*	ftFont = new CPDFFTFont( *this, fontBuffer, fontBufferLen, fIndex );
                        retFont = reinterpret_cast< CPDFFont* >( ftFont );
                    } else {
                        // mLog.Debug( boost::format("FTError: '%d'\n") % dfErr );
                    }
                }
            } else {
            // mLog.Debug( boost::format("Unable to locate a matching font for '%s'\n") % pszFontname );
            }

            podofo_free( path );
            CFRelease( pathRef );
            CFRelease( url );
        }
    } else {
        // mLog.Debug( boost::format("Unable to locate a matching font for '%s'\n") % pszFontname );
    }
}

#endif // apple

double PdfFontMetrics::StringWidth( const char* pszText, pdf_long nLength ) const
{
    double dWidth = 0.0;

    if( !pszText )
        return dWidth;

    if( !nLength )
        nLength = strlen( pszText );

    const char *localText = pszText;
    for ( pdf_long i=0; i<nLength; i++ )
    {
        dWidth += CharWidth( *localText );
        if (*localText == 0x20)
            dWidth += m_fWordSpace * this->GetFontScale() / 100.0;
        localText++;
    }

    return dWidth;
}

double PdfFontMetrics::StringWidth( const pdf_utf16be* pszText, unsigned int nLength ) const
{
    double dWidth = 0.0;
    unsigned short uChar;

    if( !pszText )
        return dWidth;

    if( !nLength )
    {
    const pdf_utf16be* pszCount = pszText;
    while( *pszCount )
    {
        ++pszCount;
        ++nLength;
    }
    }

    const pdf_utf16be* localText = pszText;
    for ( unsigned int i=0; i<nLength; i++ )
    {
#ifdef PODOFO_IS_LITTLE_ENDIAN
        uChar = static_cast<unsigned short>(((*localText & 0x00ff) << 8 | (*localText & 0xff00) >> 8));
#else
        uChar = static_cast<unsigned short>(*localText);
#endif // PODOFO_IS_LITTLE_ENDIAN
        dWidth += UnicodeCharWidth( uChar );
        if ( uChar == 0x0020 )
            dWidth += m_fWordSpace * this->GetFontScale() / 100.0;
        localText++;
    }

    return dWidth;
}

#ifndef _WCHAR_T_DEFINED
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
double PdfFontMetrics::StringWidth( const wchar_t* pszText, unsigned int nLength ) const
{
    double dWidth = 0.0;

    if( !pszText )
        return dWidth;

    if( !nLength )
        nLength = static_cast<unsigned int>(wcslen( pszText ));

    const wchar_t *localText = pszText;
    for ( unsigned int i=0; i<nLength; i++ )
    {
        dWidth += CharWidth( static_cast<int>(*localText) );
        if ( static_cast<int>(*localText) == 0x0020 )
            dWidth += m_fWordSpace * this->GetFontScale() / 100.0;
        localText++;
    }

    return dWidth;
}
#endif
#endif

EPdfFontType PdfFontMetrics::FontTypeFromFilename( const char* pszFilename )
{
    EPdfFontType eFontType = PdfFontFactory::GetFontType( pszFilename );

    if( eFontType == ePdfFontType_Unknown )
        PdfError::DebugMessage( "Warning: Unrecognized FontFormat: %s\n", pszFilename );

    return eFontType;
}

};
