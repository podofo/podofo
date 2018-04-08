/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <podofo.h>

using namespace PoDoFo;

#include <cstdlib>
#include <cstdio>

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void encrypt( const char* pszInput, const char* pszOutput, 
              const std::string & userPass, const std::string & ownerPass,
              const PdfEncrypt::EPdfEncryptAlgorithm eAlgorithm, const int nPermissions ) 
{
    PdfVecObjects objects;
    PdfParser     parser( &objects );
    
    objects.SetAutoDelete( true );
    parser.ParseFile( pszInput );
    
    PdfEncrypt::EPdfKeyLength eKeyLength;
    EPdfVersion   eVersion;
    switch( eAlgorithm ) 
    {
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncrypt::ePdfEncryptAlgorithm_RC4V1:
            eKeyLength = PdfEncrypt::ePdfKeyLength_40;
            eVersion   = ePdfVersion_1_3;
            break;
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
#ifdef PODOFO_HAVE_LIBIDN
        case PdfEncrypt::ePdfEncryptAlgorithm_AESV3:;
            eKeyLength = PdfEncrypt::ePdfKeyLength_256;
            eVersion   = ePdfVersion_1_7;
            break;
#endif // PODOFO_HAVE_LIBIDN
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncrypt::ePdfEncryptAlgorithm_RC4V2:
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncrypt::ePdfEncryptAlgorithm_AESV2:
        default:
            eKeyLength = PdfEncrypt::ePdfKeyLength_128;
            eVersion   = ePdfVersion_1_5;
            break;
    }

    PdfWriter writer( &parser );
	PdfEncrypt *encrypt = PdfEncrypt::CreatePdfEncrypt( userPass, ownerPass, nPermissions,
                        eAlgorithm, eKeyLength  );
    
    writer.SetPdfVersion( eVersion );
    writer.SetEncrypted( *encrypt );
    writer.Write( pszOutput );

	delete encrypt;
}

void print_help()
{
    printf("Usage: podofoencrypt [--rc4v1] [--rc4v2] [--aesv2] [--aesv3] [-u <userpassword>]\n");
    printf("                     -o <ownerpassword> <inputfile> <outputfile>\n\n");
    printf("       This tool encrypts an existing PDF file.\n\n");
    printf("       --help        Display this help text\n");
    printf(" Algorithm:\n");
    printf("       --rc4v1       Use rc4v1 encryption\n");
    printf("       --rc4v2       Use rc4v2 encryption (Default value)\n");
    printf("       --aesv2       Use aes-128 encryption\n");
    printf("       --aesv3       Use aes-256 encryption\n");
    printf(" Passwords:\n");
    printf("       -u <password> An optional userpassword\n");
    printf("       -o <password> The required owner password\n");
    printf(" Permissions:\n");
    printf("       --print       Allow printing the document\n");
    printf("       --edit        Allow modifying the document besides annotations, form fields or changing pages\n");
    printf("       --copy        Allow text and graphic extraction\n");
    printf("       --editnotes   Add or modify text annoations or form fields (if ePdfPermissions_Edit is set also allow the creation interactive form fields including signature)\n");
    printf("       --fillandsign Fill in existing form or signature fields\n");
    printf("       --accessible  Extract text and graphics to support user with disabillities\n");
    printf("       --assemble    Assemble the document: insert, create, rotate delete pages or add bookmarks\n");
    printf("       --highprint   Print a high resolution version of the document\n");
    printf("\n\n");
}
       
int main( int argc, char* argv[] )
{
  const char*                      pszInput   = NULL;
  const char*                      pszOutput  = NULL;
  PdfEncrypt::EPdfEncryptAlgorithm eAlgorithm = PdfEncrypt::ePdfEncryptAlgorithm_AESV2;
  int                              nPerm      = 0;
  std::string                      userPass;
  std::string                      ownerPass;

  if( argc < 3 )
  {
    print_help();
    exit( -1 );
  }

  // Parse the commandline options
  for( int i=1;i<argc;i++ ) 
  {
      if( argv[i][0] == '-' ) 
      {
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
          if( strcmp( argv[i], "--rc4v1" ) == 0 ) 
              eAlgorithm = PdfEncrypt::ePdfEncryptAlgorithm_RC4V1;
          else if( strcmp( argv[i], "--rc4v2" ) == 0 ) 
              eAlgorithm = PdfEncrypt::ePdfEncryptAlgorithm_RC4V2;
          else
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
          if( strcmp( argv[i], "--aesv2" ) == 0 ) 
              eAlgorithm = PdfEncrypt::ePdfEncryptAlgorithm_AESV2;
#ifdef PODOFO_HAVE_LIBIDN
          else if( strcmp( argv[i], "--aesv3" ) == 0 ) 
              eAlgorithm = PdfEncrypt::ePdfEncryptAlgorithm_AESV3;
#endif // PODOFO_HAVE_LIBIDN
          else if( strcmp( argv[i], "-u" ) == 0 ) 
          {
              ++i;
              if( i < argc ) 
                  userPass = argv[i];
              else
              {
                  fprintf( stderr, "ERROR: -u given on the commandline but no userpassword!\n");
                  exit( -1 );
              }
          }
          else if( strcmp( argv[i], "-o" ) == 0 ) 
          {
              ++i;
              if( i < argc ) 
                  ownerPass = argv[i];
              else
              {
                  fprintf( stderr, "ERROR: -o given on the commandline but no ownerpassword!\n");
                  exit( -1 );
              }
          }
          else if( strcmp( argv[i], "--help" ) == 0 ) 
          {
              print_help();
              exit( -1 );
          }
          else if( strcmp( argv[i], "--print" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_Print;
          else if( strcmp( argv[i], "--edit" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_Edit;
          else if( strcmp( argv[i], "--copy" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_Copy;
          else if( strcmp( argv[i], "--editnotes" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_EditNotes;
          else if( strcmp( argv[i], "--fillandsign" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_FillAndSign;
          else if( strcmp( argv[i], "--accessible" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_Accessible;
          else if( strcmp( argv[i], "--assemble" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_DocAssembly;
          else if( strcmp( argv[i], "--highprint" ) == 0 ) 
              nPerm |= PdfEncrypt::ePdfPermissions_HighPrint;
          else
          {
              fprintf( stderr, "WARNING: Do not know what to do with argument: %s\n", argv[i] );
          }
      }
      else
      {
          if( !pszInput )
          {
              pszInput = argv[i];
          }
          else if( !pszOutput )
          {
              pszOutput = argv[i];
          }
          else
          {
              fprintf( stderr, "WARNING: Do not know what to do with argument: %s\n", argv[i] );
          }

      }
  }

  // Check for errors in the commandline options
  if( !pszInput ) 
  {
      fprintf( stderr, "ERROR: No input file specified\n");
      exit( -1 );
  }

  if( !pszOutput )
  {
      fprintf( stderr, "ERROR: No output file specified\n");
      exit( -1 );
  }

  if( !ownerPass.length() )
  {
      fprintf( stderr, "ERROR: No owner password specified\n");
      exit( -1 );
  }
      

  // Do the actual encryption
  try {
      encrypt( pszInput, pszOutput, userPass, ownerPass, eAlgorithm, nPerm );
  } catch( PdfError & e ) {
      fprintf( stderr, "Error: An error %i ocurred during encrypting the pdf file.\n", e.GetError() );
      e.PrintErrorMsg();
      return e.GetError();
  }


  printf("%s was successfully encrypted to: %s\n", pszInput, pszOutput );
  
  return 0;
}

